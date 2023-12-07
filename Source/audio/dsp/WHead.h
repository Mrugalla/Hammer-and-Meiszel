#pragma once
#include "../Using.h"

namespace dsp
{
	template<size_t Size>
	struct WHead
	{
		WHead();

		/* delaySizeSamples */
		void prepare(int) noexcept;

		/* numSamples */
		void operator()(int) noexcept;

		int operator[](int) const noexcept;

		const int* data() const noexcept;

		int* data() noexcept;

		// shift, numSamples
		void shift(int, int) noexcept;
	protected:
		std::array<int, Size> buf;
		int wHead, delaySize;
	};

	using WHead1x = WHead<BlockSize>;
	using WHead2x = WHead<BlockSize2x>;
	using WHead4x = WHead<BlockSize4x>;
}