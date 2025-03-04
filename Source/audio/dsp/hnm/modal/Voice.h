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
					double = 0., double = 0., double = 0., double = 0., double = 0.,
					double = 0., double = 0., double = 0., double = 0., double = 0.);

				const Parameter& operator[](int i) const noexcept;

				std::array<Parameter, kNumParams> params;
			};

			struct ParameterProcessor
			{
				// default value
				ParameterProcessor(double = 0.);

				// p, envGenVal, min, max, numChannels
				void reset(const Parameter&, double,
					double, double, int) noexcept;

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

			// samples, dualMaterial, parameters, envGenMod, numChannels, numSamples
			void operator()(double**, const DualMaterial&,
				const Parameters&, double, int, int) noexcept;

			// xen, transposeSemi, numChannels
			void setTranspose(const arch::XenManager&, double, int) noexcept;

			// xen, numChannels
			void triggerXen(const arch::XenManager&, int) noexcept;

			// xen, noteNumber, numChannels
			void triggerNoteOn(const arch::XenManager&,
				double, int) noexcept;

			void triggerNoteOff() noexcept;

			// xen, pitchbend, numChannels
			void triggerPitchbend(const arch::XenManager&,
				double, int) noexcept;

			void reportMaterialUpdate() noexcept;

			bool isRinging() const noexcept;

		private:
			MaterialDataStereo materialStereo;
			std::array<ParameterProcessor, kNumParams> parameters;
			ResonatorBank resonatorBank;
			bool wantsMaterialUpdate, snapParameterValues;

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