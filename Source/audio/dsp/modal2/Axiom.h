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
	}
}