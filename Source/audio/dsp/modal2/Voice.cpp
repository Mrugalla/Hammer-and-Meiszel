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

		Voice::SmoothStereoParameter::Param::Param() :
			prm(0.),
			val(0.)
		{}

		void Voice::SmoothStereoParameter::Param::prepare(double sampleRate, double smoothLenMs) noexcept
		{
			prm.prepare(sampleRate, smoothLenMs);
			val = 0.;
		}

		bool Voice::SmoothStereoParameter::Param::operator()(const Parameter& p, double envVal,
			double min, double max, double direc) noexcept
		{
			const auto cVal = val;
			const auto nVal = p.val + p.breite * direc;
			const auto info = prm(nVal) + envVal;
			val = math::limit(min, max, info);
			const auto dif = val - cVal;
			const auto distSquared = dif * dif;
			return distSquared > 1e-4;
		}

		//

		Voice::SmoothStereoParameter::SmoothStereoParameter() :
			params()
		{}

		void Voice::SmoothStereoParameter::prepare(double sampleRate, double smoothLenMs) noexcept
		{
			for (auto& param : params)
				param.prepare(sampleRate, smoothLenMs);
		}

		bool Voice::SmoothStereoParameter::operator()(const Parameter& p, double envGenVal,
			double min, double max, int numChannels) noexcept
		{
			const auto envVal = envGenVal * p.env;
			bool smooth = params[0](p, envVal, min, max, -1.);
			if (numChannels == 2)
				if (params[1](p, envVal, min, max, 1.))
					smooth = true;
			return smooth;
		}

		double Voice::SmoothStereoParameter::operator[](int ch) const noexcept
		{
			return params[ch].val;
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
			const auto smoothLenMs = 14.;
			for (auto i = 0; i < kNumParams; ++i)
				parameters[i].prepare(sampleRate, smoothLenMs);
		}

		void Voice::operator()(const DualMaterial& dualMaterial, const Parameters& _parameters,
			double** samples, const MidiBuffer& midi, const arch::XenManager& xen,
			const double envGenMod, double transposeSemi,
			int numChannels, int numSamples) noexcept
		{
			updateParameters(dualMaterial, _parameters, envGenMod, numChannels);
			resonatorBank
			(
				materialStereo, samples, midi, xen,
				transposeSemi, numChannels, numSamples
			);
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

			const bool blendSmooth = parameters[kBlend](_parameters[kBlend], envGenValue, 0., 1., numChannels);
			const bool spreziSmooth = parameters[kSpreizung](_parameters[kSpreizung], envGenValue, SpreizungMin, SpreizungMax, numChannels);
			const bool harmonieSmooth = parameters[kHarmonie](_parameters[kHarmonie], envGenValue, 0., 1., numChannels);

			if (blendSmooth || spreziSmooth || harmonieSmooth || wantsMaterialUpdate)
			{
				const auto& mat0 = dualMaterial[0].peakInfos;
				const auto& mat1 = dualMaterial[1].peakInfos;
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto& material = materialStereo[ch];

					const auto blend = parameters[kBlend][ch];
					const auto spreizung = parameters[kSpreizung][ch];
					const auto sprezi = std::exp(spreizung);
					const auto harmonie = parameters[kHarmonie][ch];
					const auto harmi = math::tanhApprox(Pi * harmonie);

					for (auto i = 0; i < NumFilters; ++i)
					{
						const auto& filt0 = mat0[i];
						const auto& filt1 = mat1[i];

						// BLEND
						const auto mag0 = filt0.mag;
						const auto mag1 = filt1.mag;
						const auto magRange = mag1 - mag0;
						const auto mag = mag0 + blend * magRange;
						material[i].mag = mag;

						const auto ratio0 = filt0.ratio;
						const auto ratio1 = filt1.ratio;
						const auto ratioRange = ratio1 - ratio0;
						const auto ratio = ratio0 + blend * ratioRange;
						material[i].ratio = ratio;

						// SPREIZUNG
						const auto iF = static_cast<double>(i);
						material[i].ratio += iF * sprezi;

						// HARMONIE
						const auto r = material[i].ratio;
						const auto rTuned = std::round(r);
						material[i].ratio = r + harmi * (rTuned - r);
					}
				}

				resonatorBank.updateFreqRatios(materialStereo, numChannels);
			}

			bool kraftSmooth = parameters[kKraft](_parameters[kKraft], envGenValue, -1., 1., numChannels);
			if (kraftSmooth || wantsMaterialUpdate)
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto& material = materialStereo[ch];

					auto kraft = parameters[kKraft][ch];
					kraft = (math::tanhApprox(Pi * kraft) + 1.) * .5;

					if (kraft != 0.)
					{
						for (auto i = 0; i < NumFilters; ++i)
						{
							const auto x = material[i].mag;
							const auto y = kraft * x / ((1. - kraft) - x + 2. * kraft * x);
							material[i].mag = y;
						}
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