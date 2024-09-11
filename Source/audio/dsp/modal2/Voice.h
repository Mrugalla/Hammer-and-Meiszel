#pragma once
#include "ResonatorBank.h"
#include "../EnvelopeGenerator.h"
#include "../PRM.h"

namespace dsp
{
	namespace modal2
	{
		struct Voice
		{
			struct Parameter
			{
				Parameter(double, double, double);

				double val, breite, env;
			};

			struct Parameters
			{
				Parameters(double = 0., double = -0., double = 0.,
					double = 0., double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0., double = 0.);

				const Parameter& operator[](int i) const noexcept;

				std::array<Parameter, kNumParams> params;
			};

			struct ParameterProcessor
			{
			public:
				ParameterProcessor(double defaultVal = 0.) :
					prms{ defaultVal, defaultVal },
					vals{ defaultVal, defaultVal }
				{}

				// sampleRate, smoothLenMs
				void prepare(double sampleRate, double smoothLenMs) noexcept
				{
					for (auto& prm : prms)
						prm.prepare(sampleRate, smoothLenMs);
					for (auto& val : vals)
						val = 0.;
				}

				// p, envGenVal, min, max, numChannels
				bool operator()(const Parameter& p, double envGenVal,
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
					if(distSquared > 1e-4)
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

				double operator[](int i) const noexcept
				{
					return vals[i];
				}

				std::array<PRMBlockD, 2> prms;
				std::array<double, 2> vals;
			};

			Voice();

			// sampleRate
			void prepare(double) noexcept;

			// dualMaterial, parameters, samples,
			// midi, xen, transposeSemi, envGenMod,
			// numChannels, numSamples
			void operator()(const DualMaterial&, const Parameters&,
				double**, const MidiBuffer&, const arch::XenManager&,
				double, double, int, int) noexcept;

			void reportMaterialUpdate() noexcept;

		private:
			MaterialDataStereo materialStereo;
			std::array<ParameterProcessor, kNumParams> parameters;
			ResonatorBank resonatorBank;
			bool wantsMaterialUpdate;

			void updatePartial(MaterialData&, const MaterialData&, const MaterialData&,
				double, double, double, double, int) noexcept;

			// dualMaterial, parameters, envGenMod, numChannels
			void updateParameters(const DualMaterial&,
				const Parameters&, double, int) noexcept;
		};
	}
}

/*

*/