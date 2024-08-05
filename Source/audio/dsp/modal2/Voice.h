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
			enum kParam{ kBlend, kSpreizung, kHarmonie, kKraft, kReso, kNumParams };

			struct Parameter
			{
				Parameter(double = 0., double = 0., double = 0.);

				double val, breite, env;
			};

			struct Parameters
			{
				std::array<Parameter, kNumParams> params; Parameters(double = -0., double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0., double = 0.);

				const Parameter& operator[](int i) const noexcept;
			};

			class SmoothStereoParameter
			{
				struct Param
				{
					Param();

					// sampleRate, smoothLenMs
					void prepare(double, double) noexcept;

					// p, env, min, max, direct[-1,1]
					// returns true if smoothing
					bool operator()(const Parameter&, double,
						double, double, double) noexcept;

					PRMBlockD prm;
					double val;
				};
			public:
				SmoothStereoParameter();

				// sampleRate, smoothLenMs
				void prepare(double, double) noexcept;

				// p, envGenVal, min, max, numChannels
				bool operator()(const Parameter&, double,
					double, double, int) noexcept;

				// ch
				double operator[](int) const noexcept;

				std::array<Param, 2> params;
			};

			Voice();

			// sampleRate
			void prepare(double) noexcept;

			// dualMaterial, parameters, samples,
			// midi, xen, envGenMod, transposeSemi,
			// numChannels, numSamples
			void operator()(const DualMaterial&, const Parameters&,
				double**, const MidiBuffer&, const arch::XenManager&,
				double, double, int, int) noexcept;

			void reportMaterialUpdate() noexcept;

		private:
			MaterialDataStereo materialStereo;
			std::array<SmoothStereoParameter, kNumParams> parameters;
			ResonatorBank resonatorBank;
			bool wantsMaterialUpdate;

			// dualMaterial, parameters, envGenMod, numChannels
			void updateParameters(const DualMaterial&,
				const Parameters&, double, int) noexcept;
		};
	}
}

/*

*/