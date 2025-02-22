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

			// samples, params, envGenMod, numChannels, numSamples, v
			void operator()(double**, const Voice::Parameters&,
				double, int, int, int) noexcept;

			// xen, noteNumber, numChannels, v
			void triggerNoteOn(const arch::XenManager&,
				double, int, int) noexcept;

			// v
			void triggerNoteOff(int) noexcept;

			// xen, pitchbend, numChannels, v
			void triggerPitchbend(const arch::XenManager&,
				double, int, int) noexcept;

			bool isRinging(int) const noexcept;

			Material& getMaterial(int) noexcept;

			const Material& getMaterial(int) const noexcept;

			ActivesArray& getActives() noexcept;

			void setSolo(bool) noexcept;

			// randSeed, mIdx
			void randomizeMaterial(arch::RandSeed&, int);

		private:
			DualMaterial materials;
			std::array<Voice, NumMPEChannels> voices;
		};
	}
}