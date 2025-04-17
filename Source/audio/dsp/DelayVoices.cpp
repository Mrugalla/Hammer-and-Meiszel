#include "DelayVoices.h"

namespace dsp
{
	CombFilterVoices::CombFilterVoices(const arch::XenManager& xen) :
		wHead(),
		combFilters{ xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen, xen },
		feedback(0.)
	{
	}

	void CombFilterVoices::prepare(double sampleRate)
	{
		for (auto i = 0; i < combFilters.size(); ++i)
			combFilters[i].prepare(sampleRate);
		wHead.prepare(combFilters[0].size);
	}

	void CombFilterVoices::synthesizeWHead(int numSamples) noexcept
	{
		wHead(numSamples);
	}

	void CombFilterVoices::updateParameters(double _feedback) noexcept
	{
		if (feedback == _feedback)
			return;
		feedback = math::sinApprox(_feedback * PiHalf);
	}

	void CombFilterVoices::operator()(double** samples, const MidiBuffer& midi,
		double retune, int numChannels, int numSamples, int v) noexcept
	{
		combFilters[v]
		(
			samples, midi, wHead.data(),
			feedback, retune, 2.,
			numChannels, numSamples
		);
	}
}