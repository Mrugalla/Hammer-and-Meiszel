#pragma once
#include "../Delay.h"
#include "../midi/MPESplit.h"
#include "../ParallelProcessor.h"
#include "../../../arch/Math.h"

namespace dsp
{
	namespace flanger
	{
		struct Flanger
		{
			Flanger(const arch::XenManager& xen) :
				wHead(),
				combFilters{ xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen },
				feedback(0.),
				fbRemapped(0.),
				sequencesPos(-1.)
			{}

			// Fs
			void prepare(double sampleRate)
			{
				for (auto i = 0; i < combFilters.size(); ++i)
					combFilters[i].prepare(sampleRate);
				wHead.prepare(combFilters[0].size);
			}

			// numSamples
			void synthesizeWHead(int numSamples) noexcept
			{
				wHead(numSamples);
			}

			void updateParameters(double _feedback, double _sequencesPos, int numChannels) noexcept
			{
				if (feedback != _feedback)
				{
					feedback = _feedback;
					fbRemapped = math::sinApprox(_feedback * PiHalf);
				}
				if(sequencesPos != _sequencesPos)
				{
					sequencesPos = _sequencesPos;
					for (auto i = 0; i < combFilters.size(); ++i)
						combFilters[i].updateParameters(sequencesPos, numChannels);
				}
			}

			// samples, midi, retune, numChannels, numSamples, voiceIdx
			void operator()(double** samples, const MidiBuffer& midi,
				double retune, double apReso,
				int numChannels, int numSamples, int v) noexcept
			{
				combFilters[v]
				(
					samples, midi, wHead.data(),
					fbRemapped, retune, apReso,
					numChannels, numSamples
				);
			}

		protected:
			WHead2x wHead;
			std::array<CombFilter, NumMPEChannels> combFilters;
			double feedback, fbRemapped, sequencesPos;
		};
	}
}

// todo
// 
//Flanger
	//params
		//damp[20, 20k]
		//LFO rate[0, 40]
		//LFO depth[0, 1]
		//chord type[+5, +7, +12, +17, +19, +24]
		//chord depth[0, 1]
		//dry gain[-inf, 0]
		//wet gain[-inf, 0]
		//flanger/phaser switch
// finished
//
//Flanger
	//params
		//oct[-3, 3]
		//semi[-12, 12]
		//feedback[0, 1]