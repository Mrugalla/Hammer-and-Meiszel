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

			Voice(const EnvelopeGenerator::Parameters&);

			// sampleRate
			void prepare(double) noexcept;

			// dualMaterial, parameters, samplesSrc, samplesDest, midi, xen, transposeSemi, numChannels, numSamples
			void operator()(const DualMaterial&, const Parameters&,
				const double**, double**,
				const MidiBuffer&, const arch::XenManager&,
				double, int, int) noexcept;

			// dualMaterial, parameters, numChannels
			void processSleepy(const DualMaterial&, const Parameters&, int) noexcept;

			// sleepy, samplesDest, numChannels, numSamples
			bool isSleepy(bool, double**, int, int) noexcept;

			void reportMaterialUpdate() noexcept;

		private:
			EnvelopeGenerator env;
			std::array<double, BlockSize> envBuffer;
			MaterialDataStereo materialStereo;
			std::array<SmoothStereoParameter, kNumParams> parameters;
			ResonatorBank resonatorBank;
			bool wantsMaterialUpdate;

			// midi, numSamples
			void synthesizeEnvelope(const MidiBuffer&, int) noexcept;

			// s, ts
			int synthesizeEnvelope(int, int) noexcept;

			// samplesSrc, samplesDest, numChannels, numSamples
			void processEnvelope(const double**, double**,
				int, int) noexcept;

			// samplesDest, numChannels, numSamples
			bool bufferSilent(double* const*, int, int) const noexcept;

			// dualMaterial, parameters, numChannels
			void updateParameters(const DualMaterial&,
				const Parameters&, int) noexcept;
		};
	}
}

/*

*/