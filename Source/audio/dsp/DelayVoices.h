#pragma once
#include "Delay.h"
#include "midi/MPESplit.h"
#include "ParallelProcessor.h"
#include "../../arch/Math.h"

namespace dsp
{
    struct CombFilterVoices
    {
		CombFilterVoices(const arch::XenManager& xen) :
			wHead(),
			combFilters{ xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen },
			feedback(0.)
		{}

		// Fs
		void prepare(double sampleRate)
		{
			for(auto i = 0; i < combFilters.size(); ++i)
				combFilters[i].prepare(sampleRate);
			wHead.prepare(combFilters[0].size);
		}

		// numSamples
		void synthesizeWHead(int numSamples) noexcept
		{
			wHead(numSamples);
		}

		void updateParameters(double _feedback) noexcept
		{
			if(feedback == _feedback)
				return;
			feedback = math::sinApprox(_feedback * PiHalf);
		}

		// samples, midi, retune, numChannels, numSamples, voiceIdx
		void operator()(double** samples, const MidiBuffer& midi,
			double retune, int numChannels, int numSamples, int v) noexcept
		{
			combFilters[v]
			(
				samples, midi, wHead.data(),
				feedback, retune,
				numChannels, numSamples
			);
		}

	protected:
		WHead2x wHead;
		std::array<CombFilter, NumMPEChannels> combFilters;
		double feedback;
	};
}