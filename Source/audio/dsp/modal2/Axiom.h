#pragma once

namespace dsp
{
	namespace modal2
	{
		static constexpr int NumFilters = 12;
		static constexpr double NumFiltersD = static_cast<double>(NumFilters);
		static constexpr double NumFiltersInv = 1. / NumFiltersD;

		enum class StatusMat
		{
			Processing,
			UpdatedMaterial,
			UpdatedProcessor
		};

		static constexpr double SpreizungMin = -2.;
		static constexpr double SpreizungMax = 2.;

		enum kParam { kSmartKeytrack, kBlend, kSpreizung, kHarmonie, kKraft, kReso, kResoDamp, kNumParams };
	}
}