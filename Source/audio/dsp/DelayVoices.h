#pragma once
#include "Delay.h"
#include "midi/MPESplit.h"
#include "ParallelProcessor.h"

namespace dsp
{
    struct CombFilterVoices
    {
		CombFilterVoices(const arch::XenManager& xen) :
			wHead(),
			combFilters{ xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen }
		{}

		// Fs
		void prepare(double sampleRate)
		{
			for(auto i = 0; i < combFilters.size(); ++i)
				combFilters[i].prepare(sampleRate);
			wHead.prepare(combFilters[0].size);
		}

		// voiceSplit, parallelProcessor, feedback, retune, numChannels, numSamples
		void operator()(const MPESplit& voiceSplit, PPMIDIBand& parallelProcessor,
			double feedback, double retune,
			int numChannels, int numSamples) noexcept
		{
			wHead(numSamples);

			for (auto v = 0; v < NumMPEChannels; ++v)
			{
				const auto& midi = voiceSplit[v + 2];
				auto band = parallelProcessor.getBand(v);
				double* samples[] = { band.l, band.r };
				combFilters[v]
				(
					samples, midi, wHead.data(),
					feedback, retune,
					numChannels, numSamples
				);
			}
		}

	protected:
		WHead2x wHead;
		std::array<CombFilter, NumMPEChannels> combFilters;
	};
}