#pragma once
#include <juce_core/juce_core.h>

namespace dsp
{
	namespace modal
	{
		static constexpr int NumFilters = 12;
		static constexpr double NumFiltersD = static_cast<double>(NumFilters);
		static constexpr double NumFiltersInv = 1. / NumFiltersD;
		static constexpr double MinPitch = 24;

		enum class StatusMat
		{
			Processing,
			UpdatedMaterial,
			UpdatedProcessor
		};

		using String = juce::String;

		String toString(StatusMat);

		static constexpr double SpreizungMin = -2.;
		static constexpr double SpreizungMax = 2.;

		enum kParam { kKeytrack, kBlend, kSpreizung, kHarmonie, kKraft, kReso, kResoDamp, kNumParams };
	}
}