#pragma once
#include "juce_core/juce_core.h"

namespace makeRange
{
	using Range = juce::NormalisableRange<float>;
	using String = juce::String;

	/* start, end, bias[-1, 1] */
	Range biased(float, float, float) noexcept;

	/* start, end, steps = 1 */
	Range stepped(float, float, float = 1.f) noexcept;

	Range toggle() noexcept;

	/* start, end */
	Range lin(float, float) noexcept;

	/* start, end, centre */
	Range withCentre(float, float, float) noexcept;

	/* min, max */
	Range foleysLogRange(float, float) noexcept;

	/* min, max, numSteps ]1, N] */
	Range quad(float, float, int) noexcept;

	/* minDenominator, maxDenominator, withZero
	for example { 16, .5, true }
	starts at 0, then 1/16 and ends at 2/1
	*/
	Range beats(float, float, bool = false);
}