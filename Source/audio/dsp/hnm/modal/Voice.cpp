#include "Voice.h"

namespace dsp
{
	namespace modal
	{
		//

		Voice::Parameter::Parameter(double _val, double _breite, double _env) :
			val(_val),
			breite(_breite),
			env(_env)
		{}

		//

		Voice::Parameters::Parameters(double _smartKeytrack, double _blend, double _spreizung, double _harmonie, double _kraft, double _reso, double _resoDamp,
			double _smartKeytrackEnv, double _blendEnv, double _spreizungEnv, double _harmonieEnv, double _kraftEnv, double _resoEnv, double _resoDampEnv,
			double _smartKeytrackBreite, double _blendBreite, double _spreizungBreite, double _harmonieBreite, double _kraftBreite, double _resoBreite, double _resoDampBreite) :
			params
			{
				Parameter(_smartKeytrack, _smartKeytrackBreite, _smartKeytrackEnv),
				Parameter(_blend, _blendBreite, _blendEnv),
				Parameter(_spreizung, _spreizungBreite, _spreizungEnv),
				Parameter(_harmonie, _harmonieBreite, _harmonieEnv),
				Parameter(_kraft, _kraftBreite, _kraftEnv),
				Parameter(_reso, _resoBreite, _resoEnv),
				Parameter(_resoDamp, _resoDampBreite, _resoDampEnv)

			}
		{}

		const Voice::Parameter& Voice::Parameters::operator[](int i) const noexcept
		{
			return params[i];
		}

		//

		Voice::ParameterProcessor::ParameterProcessor(double defaultVal) :
			prms{ defaultVal, defaultVal },
			vals{ defaultVal, defaultVal }
		{}

		// sampleRate, smoothLenMs
		void Voice::ParameterProcessor::prepare(double sampleRate, double smoothLenMs) noexcept
		{
			for (auto& prm : prms)
				prm.prepare(sampleRate, smoothLenMs);
			for (auto& val : vals)
				val = 0.;
		}

		// p, envGenVal, min, max, numChannels
		bool Voice::ParameterProcessor::operator()(const Parameter& p, double envGenVal,
			double min, double max, int numChannels) noexcept
		{
			bool smooth = false;
			const auto env = p.env * envGenVal;

			auto lastVal = vals[0];
			auto nVal = p.val + p.breite;
			auto smVal = prms[0](nVal);
			vals[0] = math::limit(min, max, smVal + env);
			auto dif = vals[0] - lastVal;
			auto distSquared = dif * dif;
			if (distSquared > 1e-4)
				smooth = true;
			if (numChannels != 2)
				return smooth;

			lastVal = vals[1];
			nVal = p.val - p.breite;
			smVal = prms[1](nVal);
			vals[1] = math::limit(min, max, smVal + env);
			dif = vals[1] - lastVal;
			distSquared = dif * dif;
			if (distSquared > 1e-4)
				smooth = true;

			return smooth;
		}

		double Voice::ParameterProcessor::operator[](int i) const noexcept
		{
			return vals[i];
		}

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
			const auto smoothLenMs = 13.;
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
				auto& resoParam = parameters[kParam::kReso];
				resoParam(_parameters[kParam::kReso], envGenValue, 0., 1., numChannels);
				auto& resoDampParam = parameters[kParam::kResoDamp];
				resoDampParam(_parameters[kParam::kResoDamp], envGenValue, 0., 1., numChannels);
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto reso = resoParam[ch];
					const auto resoDamp = resoDampParam[ch];
					resonatorBank.setReso(reso, resoDamp, ch);
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