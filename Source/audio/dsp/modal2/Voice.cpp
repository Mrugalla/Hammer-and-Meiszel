#include "Voice.h"

namespace dsp
{
	namespace modal2
	{
		//

		Voice::Parameter::Parameter(double _val, double _breite, double _env) :
			val(_val),
			breite(_breite),
			env(_env)
		{}

		//

		Voice::Parameters::Parameters(double _smartKeytrack, double _blend, double _spreizung, double _harmonie, double _kraft, double _reso,
			double _smartKeytrackEnv, double _blendEnv, double _spreizungEnv, double _harmonieEnv, double _kraftEnv, double _resoEnv,
			double _smartKeytrackBreite, double _blendBreite, double _spreizungBreite, double _harmonieBreite, double _kraftBreite, double _resoBreite) :
			params
			{
				Parameter(_smartKeytrack, _smartKeytrackBreite, _smartKeytrackEnv),
				Parameter(_blend, _blendBreite, _blendEnv),
				Parameter(_spreizung, _spreizungBreite, _spreizungEnv),
				Parameter(_harmonie, _harmonieBreite, _harmonieEnv),
				Parameter(_kraft, _kraftBreite, _kraftEnv),
				Parameter(_reso, _resoBreite, _resoEnv)
			}
		{}

		const Voice::Parameter& Voice::Parameters::operator[](int i) const noexcept
		{
			return params[i];
		}

		//

		//

		Voice::Voice() :
			materialStereo(),
			parameters(),
			resonatorBank(),
			wantsMaterialUpdate(true)
		{}

		void Voice::prepare(double sampleRate) noexcept
		{
			resonatorBank.prepare(materialStereo, sampleRate);
			const auto smoothLenMs = 12.;
			for (auto i = 0; i < kNumParams; ++i)
				parameters[i].prepare(sampleRate, smoothLenMs);
		}

		void Voice::operator()(const DualMaterial& dualMaterial, const Parameters& _parameters,
			double** samples, const MidiBuffer& midi, const arch::XenManager& xen,
			double transposeSemi, double envGenMod, int numChannels, int numSamples) noexcept
		{
			updateParameters(dualMaterial, _parameters, envGenMod, numChannels);
			resonatorBank
			(
				materialStereo, samples, midi, xen,
				transposeSemi, numChannels, numSamples
			);
		}

		void Voice::updatePartial(MaterialData& dest, const MaterialData& src0, const MaterialData& src1,
			double smartKeytrack, double blend, double sprezi, double harmi, int i) noexcept
		{
			const auto& filt0 = src0[i];
			const auto& filt1 = src1[i];

			// BLEND
			const auto mag0 = filt0.mag;
			const auto mag1 = filt1.mag;
			const auto magRange = mag1 - mag0;
			const auto mag = mag0 + blend * magRange;
			dest[i].mag = mag;

			const auto freqHz0 = filt0.freqHz;
			const auto freqHz1 = filt1.freqHz;
			const auto freqHzRange = freqHz1 - freqHz0;
			dest[i].freqHz = freqHz0 + blend * freqHzRange;

			const auto keytrack0 = filt0.keytrack;
			const auto keytrack1 = filt1.keytrack;
			const auto keytrackRange = keytrack1 - keytrack0;
			const auto keyTrackMix = keytrack0 + blend * keytrackRange;
			dest[i].keytrack = 1. + smartKeytrack * (keyTrackMix - 1.);

			const auto ratio0 = filt0.ratio;
			const auto ratio1 = filt1.ratio;
			const auto ratioRange = ratio1 - ratio0;
			const auto ratio = ratio0 + blend * ratioRange;
			dest[i].ratio = ratio;

			// SPREIZUNG
			const auto iF = static_cast<double>(i);
			dest[i].ratio += iF * sprezi;

			// HARMONIE
			const auto r = dest[i].ratio;
			const auto rTuned = std::round(r);
			dest[i].ratio = r + harmi * (rTuned - r);
		}

		void Voice::updateParameters(const DualMaterial& dualMaterial,
			const Parameters& _parameters, double envGenMod, int numChannels) noexcept
		{
			const auto envGenValue = envGenMod;

			{
				auto& resoParam = parameters[kReso];
				const auto resoSmooth = resoParam(_parameters[kReso], envGenValue, 0., 1., numChannels);
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto reso = resoParam[ch];
					resonatorBank.setReso(reso, ch);
				}
			}

			auto& smartKeytrackParam = parameters[kSmartKeytrack];
			const bool smartKeytrackSmooth = smartKeytrackParam(_parameters[kSmartKeytrack], envGenValue, 0., 1., numChannels);
			auto& blendParam = parameters[kBlend];
			const bool blendSmooth = blendParam(_parameters[kBlend], envGenValue, 0., 1., numChannels);
			auto& spreziParam = parameters[kSpreizung];
			const bool spreziSmooth = spreziParam(_parameters[kSpreizung], envGenValue, SpreizungMin, SpreizungMax, numChannels);
			auto& harmonieParam = parameters[kHarmonie];
			const bool harmonieSmooth = harmonieParam(_parameters[kHarmonie], envGenValue, 0., 1., numChannels);

			if (smartKeytrackSmooth || blendSmooth || spreziSmooth || harmonieSmooth || wantsMaterialUpdate)
			{
				const auto& mat0 = dualMaterial.getMaterialData(0);
				const auto& mat1 = dualMaterial.getMaterialData(1);
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto& material = materialStereo[ch];

					const auto smartKeytrack = smartKeytrackParam[ch];
					const auto blend = blendParam[ch];
					const auto spreizung = spreziParam[ch];
					const auto sprezi = std::exp(spreizung);
					const auto harmonie = harmonieParam[ch];
					const auto harmi = math::tanhApprox(Pi * harmonie);

					updatePartial(material, mat0, mat1, smartKeytrack, blend, sprezi, harmi, 0);
					for (auto i = 1; i < NumFilters; ++i)
					{
						if (dualMaterial.isActive(i))
							updatePartial(material, mat0, mat1, smartKeytrack, blend, sprezi, harmi, i);
						else
							material[i].mag = 0.;
					}
				}
				resonatorBank.updateFreqRatios(materialStereo, numChannels);
			}

			auto& kraftParam = parameters[kKraft];
			const bool kraftSmooth = kraftParam(_parameters[kKraft], envGenValue, -1., 1., numChannels);
			if (kraftSmooth || wantsMaterialUpdate)
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto& material = materialStereo[ch];

					const auto kraftVal = kraftParam[ch];
					const auto kraft = (math::tanhApprox(Pi * kraftVal) + 1.) * .5;

					if (kraft != 0.)
						for (auto i = 0; i < NumFilters; ++i)
						{
							const auto x = material[i].mag;
							const auto y = kraft * x / ((1. - kraft) - x + 2. * kraft * x);
							material[i].mag = y;
						}
				}
			}
		}

		void Voice::reportMaterialUpdate() noexcept
		{
			wantsMaterialUpdate = true;
		}
	}
}