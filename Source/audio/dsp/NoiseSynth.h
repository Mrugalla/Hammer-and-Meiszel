#pragma once
#include "../Using.h"

namespace dsp
{
	struct NoiseSynth
	{
		static constexpr int Size = 1 << 12;
		static constexpr int Max = Size - 1;
		static constexpr int FlipLen = 8;

		NoiseSynth();

		// samples, blend[0, 1], numChannels, numSamples
		void operator()(double**, double, int, int) noexcept;

	protected:
		Random rand;
		std::array<double, Size> noise;
		int rHead, flipIdx;
	};
}