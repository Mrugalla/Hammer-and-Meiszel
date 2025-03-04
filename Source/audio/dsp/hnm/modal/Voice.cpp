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

		Voice::Parameters::Parameters(double _blend, double _spreizung, double _harmonie, double _kraft, double _reso,
			double _blendEnv, double _spreizungEnv, double _harmonieEnv, double _kraftEnv, double _resoEnv,
			double _blendBreite, double _spreizungBreite, double _harmonieBreite, double _kraftBreite, double _resoBreite) :
			params
			{
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

		Voice::ParameterProcessor::ParameterProcessor(double defaultVal) :
			prms{ defaultVal, defaultVal },
			vals{ defaultVal, defaultVal }
		{}

		void Voice::ParameterProcessor::reset(const Parameter& p, double envGenVal,
			double min, double max, int numChannels) noexcept
		{
			const auto env = p.env * envGenVal;
			
			const auto val0 = math::limit(min, max, p.val - p.breite + env);
			prms[0].reset(val0);
			
			if (numChannels == 1)
				return;

			const auto val1 = math::limit(min, max, p.val + p.breite + env);
			prms[1].reset(val1);
		}

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
			wantsMaterialUpdate(true),
			snapParameterValues(false)
		{}

		void Voice::prepare(double sampleRate) noexcept
		{
			resonatorBank.prepare(materialStereo, sampleRate);
			const auto smoothLenMs = 42.;
			for (auto i = 0; i < kNumParams; ++i)
				parameters[i].prepare(sampleRate, smoothLenMs);
			wantsMaterialUpdate = true;
			snapParameterValues = false;
		}

		void Voice::operator()(double** samples, const DualMaterial& dualMaterial,
			const Parameters& params, double envGenMod, int numChannels, int numSamples) noexcept
		{
			updateParameters(dualMaterial, params, envGenMod, numChannels);
			resonatorBank(materialStereo, samples, numChannels, numSamples);
		}

		void Voice::setTranspose(const arch::XenManager& xen, double transposeSemi, int numChannels) noexcept
		{
			resonatorBank.setTranspose(materialStereo, xen, transposeSemi, numChannels);
		}

		void Voice::triggerXen(const arch::XenManager& xen, int numChannels) noexcept
		{
			resonatorBank.triggerXen(xen, materialStereo, numChannels);
		}

		void Voice::triggerNoteOn(const arch::XenManager& xen, double noteNumber, int numChannels) noexcept
		{
			resonatorBank.triggerNoteOn(materialStereo, xen, noteNumber, numChannels);
			snapParameterValues = true;
		}

		void Voice::triggerNoteOff() noexcept
		{
			resonatorBank.triggerNoteOff();
		}

		void Voice::triggerPitchbend(const arch::XenManager& xen,
			double pitchbend, int numChannels) noexcept
		{
			resonatorBank.triggerPitchbend(materialStereo, xen, pitchbend, numChannels);
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

			auto& resoParam = parameters[kParam::kReso];
			auto& blendParam = parameters[kBlend];
			auto& spreziParam = parameters[kSpreizung];
			auto& harmonieParam = parameters[kHarmonie];
			auto& kraftParam = parameters[kKraft];

			bool wannaUpdate = wantsMaterialUpdate;
			if (snapParameterValues)
			{
				resoParam.reset(_parameters[kParam::kReso], envGenValue, 0., 1., numChannels);
				blendParam.reset(_parameters[kBlend], envGenValue, 0., 1., numChannels);
				spreziParam.reset(_parameters[kSpreizung], envGenValue, SpreizungMin, SpreizungMax, numChannels);
				harmonieParam.reset(_parameters[kHarmonie], envGenValue, 0., 1., numChannels);
				kraftParam.reset(_parameters[kKraft], envGenValue, -1., 1., numChannels);
				wannaUpdate = true;
				snapParameterValues = false;
			}
			
			resoParam(_parameters[kParam::kReso], envGenValue, 0., 1., numChannels);
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto reso = resoParam[ch];
				resonatorBank.setReso(reso, ch);
			}
			const bool blendSmooth = blendParam(_parameters[kBlend], envGenValue, 0., 1., numChannels);
			const bool spreziSmooth = spreziParam(_parameters[kSpreizung], envGenValue, SpreizungMin, SpreizungMax, numChannels);
			const bool harmonieSmooth = harmonieParam(_parameters[kHarmonie], envGenValue, 0., 1., numChannels);
			const bool kraftSmooth = kraftParam(_parameters[kKraft], envGenValue, -1., 1., numChannels);

			if (wannaUpdate || blendSmooth || spreziSmooth || harmonieSmooth || kraftSmooth)
			{
				const auto& mat0 = dualMaterial.getMaterialData(0);
				const auto& mat1 = dualMaterial.getMaterialData(1);

				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto& material = materialStereo[ch];

					const auto blend = blendParam[ch];
					const auto spreizung = spreziParam[ch];
					const auto sprezi = std::exp(spreizung);
					const auto harmonie = harmonieParam[ch];
					const auto harmi = math::tanhApprox(Pi * harmonie);

					for (auto i = 0; i < NumPartials; ++i)
					{
						if (dualMaterial.isActive(i))
						{
							blendMags(material, mat0, mat1, blend, i);
							blendRatios(material, mat0, mat1, blend, i);
							updatePartial(material, sprezi, harmi, i);
						}
						else
							material[i].mag = 0.;
					}

					const auto kraftVal = kraftParam[ch];
					const auto kraft = (math::tanhApprox(Pi * kraftVal) + 1.) * .5;

					if (kraftVal != 0.)
						for (auto i = 0; i < NumPartials; ++i)
						{
							const auto x = material[i].mag;
							const auto y = kraft * x / ((1. - kraft) - x + 2. * kraft * x);
							material[i].mag = y;
						}
				}
				resonatorBank.updateFreqRatios(materialStereo, numChannels);
			}
			wantsMaterialUpdate = false;
		}

		void Voice::reportMaterialUpdate() noexcept
		{
			wantsMaterialUpdate = true;
		}
	}
}