#pragma once
#include "juce_dsp/juce_dsp.h"
//#include <complex>
#include "../Using.h"

namespace dsp
{
	struct FFT
	{
		static constexpr int Order = 9;
		static constexpr int Size = 1 << Order;
		static constexpr int Size2 = Size * 2;
		static constexpr float Size2Inv = 1.f / Size2;
		static constexpr float Size2InvTau = Size2Inv * TauF;
	
		FFT() :
			fft(Order),
			fifo(),
			idx(0)
		{}

		void operator()(float* smpls, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				processSample(smpls[s]);
		}

		float processSample(float x) noexcept
		{
			fifo[idx] = x;
			if (++idx == Size)
			{
				idx = 0;
				applyFFT();
			}
			return x;
		}

	protected:
		juce::dsp::FFT fft;
		std::array<float, Size2> fifo;
		int idx;

		void applyFFT() noexcept
		{
			fft.performRealOnlyForwardTransform(fifo.data(), true);
		}
	};

	inline void fftTest()
	{
		FFT fft;
	}
}