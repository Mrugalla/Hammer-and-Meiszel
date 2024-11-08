#pragma once
#include "Voice.h"

namespace dsp
{
	namespace modal
	{
		struct ModalFilter
		{
			ModalFilter();

			// sampleRate
			void prepare(double) noexcept;

			void operator()() noexcept;

			// samples, midi, xen, voiceParams, transposeSemi, envGenMod, numChannels, numSamples, v
			void operator()(double**, const MidiBuffer&, const arch::XenManager&,
				const Voice::Parameters&, double, double, int, int, int) noexcept;

			Material& getMaterial(int) noexcept;

			const Material& getMaterial(int) const noexcept;

			ActivesArray& getActives() noexcept;

			void setSolo(bool) noexcept;

		private:
			DualMaterial materials;
			std::array<Voice, NumMPEChannels> voices;
		};
	}
}