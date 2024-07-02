#pragma once

namespace dsp
{
	namespace modal2
	{
		static constexpr int NumFilters = 14;

		enum class Status
		{
			Processing,
			UpdatedMaterial,
			UpdatedProcessor
		};

		static constexpr double SpreizungMin = -2.;
		static constexpr double SpreizungMax = 2.;
	}
}