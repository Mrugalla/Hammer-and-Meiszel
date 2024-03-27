#pragma once

namespace dsp
{
	namespace modal
	{
#if JUCE_DEBUG
		static constexpr int NumFilters = 12;
#else
		static constexpr int NumFilters = 16;
#endif
		enum class Status
		{
			Processing,
			UpdatedMaterial,
			UpdatedDualMaterial,
		};
	}
}