#include "WHead.h"

namespace dsp
{
	template<size_t Size>
	WHead<Size>::WHead() :
		buf(),
		wHead(0),
		delaySize(1)
	{}

	template<size_t Size>
	void WHead<Size>::prepare(int _delaySize) noexcept
	{
		delaySize = _delaySize;
		if (delaySize != 0)
			wHead = wHead % delaySize;
	}

	template<size_t Size>
	void WHead<Size>::operator()(int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s, wHead = (wHead + 1) % delaySize)
			buf[s] = wHead;
	}

	template<size_t Size>
	int WHead<Size>::operator[](int i) const noexcept
	{
		return buf[i];
	}

	template<size_t Size>
	const int* WHead<Size>::data() const noexcept
	{
		return buf.data();
	}

	template<size_t Size>
	int* WHead<Size>::data() noexcept
	{
		return buf.data();
	}

	template<size_t Size>
	void WHead<Size>::shift(int shift, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
		{
			buf[s] = buf[s] + shift;
			if (buf[s] > delaySize)
				buf[s] -= delaySize;
			else if (buf[s] < 0)
				buf[s] += delaySize;
		}
		wHead = buf[numSamples - 1];
	}

	template struct WHead<BlockSize>;
	template struct WHead<BlockSize2x>;
	template struct WHead<BlockSize4x>;
}