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

		Voice::Parameters::Parameters(double _blend, double _spreizung, double _harmonie, double _kraft, double _reso, double _resoDamp,
			double _blendEnv, double _spreizungEnv, double _harmonieEnv, double _kraftEnv, double _resoEnv, double _resoDampEnv,
			double _blendBreite, double _spreizungBreite, double _harmonieBreite, double _kraftBreite, double _resoBreite, double _resoDampBreite) :
			params
			{
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
			for (auto ch = 0; ch < 2; ++ch)
			{
				auto& prm = prms[ch];
				prm.prepare(sampleRate, smoothLenMs);
				auto& val = vals[ch];
				val = prm.startVal;
			}
		}

		// p, envGenVal, min, max, numChannels
		bool Voice::ParameterProcessor::operator()(const Parameter& p, double envGenVal,
			double min, double max, int numChannels) noexcept
		{
			bool smooth = false;
			const auto env = p.env * envGenVal;

			const auto eps = 1e-10;

			auto lastVal = vals[0];
			auto nVal = p.val - p.breite;
			auto smVal = prms[0](nVal);
			vals[0] = math::limit(min, max, smVal + env);
			auto dif = vals[0] - lastVal;
			auto distAbs = std::abs(dif);
			if (distAbs > eps)
				smooth = true;
			if (numChannels == 1)
				return smooth;

			lastVal = vals[1];
			nVal = p.val + p.breite;
			smVal = prms[1](nVal);
			vals[1] = math::limit(min, max, smVal + env);
			dif = vals[1] - lastVal;
			distAbs = std::abs(dif);
			if (distAbs > eps)
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
			const auto smoothLenMs = 42.;
			for (auto i = 0; i < kNumParams; ++i)
				parameters[i].prepare(sampleRate, smoothLenMs);
			wantsMaterialUpdate = true;
		}

		void Voice::operator()(const DualMaterial& dualMaterial, const Parameters& _parameters,
			double** samples, const MidiBuffer& midi, const arch::XenManager& xen,
			double transposeSemi, double envGenMod, int numChannels, int numSamples) noexcept
		{
			updateParameters
			(
				dualMaterial,
				_parameters,
				envGenMod,
				numChannels
			);
			resonatorBank
			(
				materialStereo, samples, midi, xen,
				transposeSemi, numChannels, numSamples
			);
		}

		bool Voice::isRinging() const noexcept
		{
			return resonatorBank.isRinging();
		}

		void Voice::blendMags(MaterialData& dest,
			const MaterialData& src0, const MaterialData& src1,
			double blend, int i) noexcept
		{
			const auto& filt0 = src0[i];
			const auto& filt1 = src1[i];

			const auto mag0 = filt0.mag;
			const auto mag1 = filt1.mag;
			const auto magRange = mag1 - mag0;
			const auto mag = mag0 + blend * magRange;
			dest[i].mag = mag;
		}

		void Voice::blendRatios(MaterialData& dest,
			const MaterialData& src0, const MaterialData& src1,
			double blend, int i) noexcept
		{
			const auto& filt0 = src0[i];
			const auto& filt1 = src1[i];

			const auto fc0 = filt0.fc;
			const auto fc1 = filt1.fc;
			const auto fcRange = fc1 - fc0;
			const auto fc = fc0 + blend * fcRange;
			dest[i].fc = fc;
		}

		void Voice::updatePartial(MaterialData& dest,
			double sprezi, double harmi, int i) noexcept
		{
			// SPREIZUNG
			const auto iF = static_cast<double>(i);
			dest[i].fc += iF * sprezi;

			// HARMONIE
			const auto r = dest[i].fc;
			const auto rTuned = std::round(r);
			dest[i].fc = r + harmi * (rTuned - r);
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
					resonatorBank.setReso(std::sqrt(reso), resoDamp, ch);
				}
			}

			auto& blendParam = parameters[kBlend];
			const bool blendSmooth = blendParam(_parameters[kBlend], envGenValue, 0., 1., numChannels);
			auto& spreziParam = parameters[kSpreizung];
			const bool spreziSmooth = spreziParam(_parameters[kSpreizung], envGenValue, SpreizungMin, SpreizungMax, numChannels);
			auto& harmonieParam = parameters[kHarmonie];
			const bool harmonieSmooth = harmonieParam(_parameters[kHarmonie], envGenValue, 0., 1., numChannels);

			if (blendSmooth || spreziSmooth || harmonieSmooth || wantsMaterialUpdate)
			{
				const auto& mat0 = dualMaterial.getMaterialData(0);
				const auto& mat1 = dualMaterial.getMaterialData(1);
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto& material = materialStereo[ch];

					const auto blend = blendParam[ch];
					for (auto i = 0; i < NumPartials; ++i)
						blendMags(material, mat0, mat1, blend, i);
					blendRatios(material, mat0, mat1, blend, 0);
					for (auto i = 1; i < NumPartials; ++i)
					{
						if (dualMaterial.isActive(i))
							blendRatios(material, mat0, mat1, blend, i);
						else
							material[i].mag = 0.;
					}

					const auto spreizung = spreziParam[ch];
					const auto sprezi = std::exp(spreizung);
					const auto harmonie = harmonieParam[ch];
					const auto harmi = math::tanhApprox(Pi * harmonie);

					for (auto i = 1; i < NumPartialsKeytracked; ++i)
						updatePartial(material, sprezi, harmi, i);
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
						for (auto i = 0; i < NumPartials; ++i)
						{
							const auto x = material[i].mag;
							const auto y = kraft * x / ((1. - kraft) - x + 2. * kraft * x);
							material[i].mag = y;
						}
				}
			}

			wantsMaterialUpdate = false;
		}

		void Voice::reportMaterialUpdate() noexcept
		{
			wantsMaterialUpdate = true;
		}
	}
}