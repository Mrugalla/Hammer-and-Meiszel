#pragma once
#include "Resonatr.h"
#include "../EnvelopeGenerator.h"

namespace dsp
{
	namespace modal
	{
		struct Voice
		{
			Voice(const arch::XenManager&, const EnvelopeGenerator::Parameters&);

			void prepare(double);

			// bwFc
			void setBandwidth(double) noexcept;

			// transposeSemi, pbRange, xen
			void updateResonatorVal(double transposeSemi, double pbRange, double xen) noexcept
			{
				filter.updateVal(transposeSemi, pbRange, xen);
			}

			// samplesSrc, samplesDest, midi, numChannels, numSamples
			void operator()(const double**, double**, const MidiBuffer&, int, int) noexcept;

			// sleepy, samplesDest, numChannels, numSamples
			void detectSleepy(bool&, double**, int, int) noexcept;

			PeakArray peaks;
			Resonatr filter;
			EnvelopeGenerator env;
			std::array<double, BlockSize> envBuffer;
			double gain;
		protected:

			// midi, numSamples
			void synthesizeEnvelope(const MidiBuffer&, int) noexcept;

			// s, ts
			int synthesizeEnvelope(int, int) noexcept;

			// samplesSrc, samplesDest, numChannels, numSamples
			void processEnvelope(const double**, double**, int, int) noexcept;

			// samplesDest, numChannels, numSamples
			bool bufferSilent(double* const*, int, int) const noexcept;
		};
	}
}