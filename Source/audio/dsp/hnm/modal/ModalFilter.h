#pragma once
#include "Voice.h"
#include "../../../../arch/RandomSeed.h"

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

			bool isRinging(int) const noexcept;

			Material& getMaterial(int) noexcept;

			const Material& getMaterial(int) const noexcept;

			ActivesArray& getActives() noexcept;

			void setSolo(bool) noexcept;

			// randSeed, xen, mIdx
			void randomizeMaterial(arch::RandSeed&, const arch::XenManager&, int);

		private:
			DualMaterial materials;
			std::array<Voice, NumMPEChannels> voices;
		};
	}
}