#pragma once
#include "Voice.h"

namespace dsp
{
	namespace modal2
	{
		struct ModalFilter
		{
			ModalFilter();

			// sampleRate
			void prepare(double) noexcept;

			void operator()(const EnvelopeGenerator::Parameters&) noexcept;

			// samplesSrc, samplesDest, midi, xen, voiceParams, transposeSemi, numChannels, numSamples, v
			void operator()(const double**, double**, const MidiBuffer&,
				const arch::XenManager&, const Voice::Parameters&, double, int, int, int) noexcept;

			// voiceParams, numChannels, v
			void processSleepy(const Voice::Parameters&, int, int) noexcept;

			// sleepy, samplesDest, numChannels, numSamples, v
			bool isSleepy(bool&, double**, int, int, int) noexcept;

			Material& getMaterial(int) noexcept;

			const Material& getMaterial(int) const noexcept;

		private:
			DualMaterial materials;
			EnvelopeGenerator::Parameters envGenParams;
			std::array<Voice, NumMPEChannels> voices;
		};
	}
}