#pragma once
#include "Delay.h"
#include "midi/MPESplit.h"
#include "ParallelProcessor.h"

namespace dsp
{
    struct CombFilterVoices
    {
		CombFilterVoices(const arch::XenManager&);

		// Fs
		void prepare(double);

		// numSamples
		void synthesizeWHead(int) noexcept;

		void updateParameters(double) noexcept;

		// samples, midi, retune, numChannels, numSamples, voiceIdx
		void operator()(double**, const MidiBuffer&,
			double, int, int, int) noexcept;

	protected:
		WHead2x wHead;
		std::array<CombFilter, NumMPEChannels> combFilters;
		double feedback;
	};
}