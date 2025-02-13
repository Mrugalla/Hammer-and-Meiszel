#pragma once
#include "ResonatorBank.h"
#include "../../EnvelopeGenerator.h"
#include "../../PRM.h"

namespace dsp
{
	namespace modal
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
				Parameters(double = 0., double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0., double = 0.);

				const Parameter& operator[](int i) const noexcept;

				std::array<Parameter, kNumParams> params;
			};

			struct ParameterProcessor
			{
				// default value
				ParameterProcessor(double = 0.);

				// sampleRate, smoothLenMs
				void prepare(double, double) noexcept;

				// p, envGenVal, min, max, numChannels
				bool operator()(const Parameter&, double,
					double, double, int) noexcept;

				double operator[](int i) const noexcept;

				std::array<PRMBlockD, 2> prms;
				std::array<double, 2> vals;
			};

			Voice();

			// sampleRate
			void prepare(double) noexcept;

			// dualMaterial, parameters,
			// samples,
			// midi, xen, transposeSemi, envGenMod,
			// numChannels, numSamples
			void operator()(const DualMaterial&, const Parameters&, double**,
				const MidiBuffer&, const arch::XenManager&,
				double, double, int, int) noexcept;

			void reportMaterialUpdate() noexcept;

			bool isRinging() const noexcept;

		private:
			MaterialDataStereo materialStereo;
			std::array<ParameterProcessor, kNumParams> parameters;
			ResonatorBank resonatorBank;
			bool wantsMaterialUpdate;

			// dest, src0 ,src1, blend, partialIdx
			void blendMags(MaterialData&, const MaterialData&, const MaterialData&, double, int) noexcept;

			// dest, src0, src1, blend, partialIdx
			void blendRatios(MaterialData&, const MaterialData&, const MaterialData&, double, int) noexcept;

			// dest, sprezi, harmi, partialIdx
			void updatePartial(MaterialData&, double, double, int) noexcept;

			// dualMaterial, parameters, envGenMod, numChannels
			void updateParameters(const DualMaterial&,
				const Parameters&, double, int) noexcept;
		};
	}
}