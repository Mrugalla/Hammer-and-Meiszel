#include "MidSide.h"

namespace dsp
{
	void midSideEncode(double* const* samples, int numSamples) noexcept
	{
		auto smplsL = samples[0];
		auto smplsR = samples[1];

		for (auto s = 0; s < numSamples; ++s)
		{
			const auto mid = (smplsL[s] + smplsR[s]) * .5;
			const auto side = (smplsL[s] - smplsR[s]) * .5;
			smplsL[s] = mid;
			smplsR[s] = side;
		}
	}

	void midSideDecode(double* const* samples, int numSamples) noexcept
	{
		auto smplsM = samples[0];
		auto smplsS = samples[1];

		for (auto s = 0; s < numSamples; ++s)
		{
			const auto left = smplsM[s] + smplsS[s];
			const auto right = smplsM[s] - smplsS[s];
			smplsM[s] = left;
			smplsS[s] = right;
		}
	}
}