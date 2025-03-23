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

			// samples, params, xen, envGenMod, numChannels, numSamples, v
			void operator()(double**, const Voice::Parameters&,
				double, int, int, int) noexcept;

			// xen, transposeSemi, numChannels
			void setTranspose(const arch::XenManager&, double, int) noexcept;

			// xen, numChannels
			void triggerXen(const arch::XenManager&, int) noexcept;

			// xen, noteNumber, numChannels, v, polyphonic
			void triggerNoteOn(const arch::XenManager&,
				double, int, int, bool) noexcept;

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
			double transposeSemi;
		};
	}
}