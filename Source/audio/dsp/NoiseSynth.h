#pragma once
#include "../Using.h"

namespace dsp
{
	struct NoiseSynth
	{
		static constexpr int Size = 1 << 12;
		static constexpr int Max = Size - 1;
		static constexpr int FlipLen = 8;

		NoiseSynth() :
			rand(),
			noise(),
			rHead(0),
			flipIdx(0)
		{
			const auto min12db = math::dbToAmp(-12.);
			for (auto i = 0; i < Size; ++i)
				noise[i] = (rand.nextDouble() * 2. - 1.) * min12db;
		}

		// samples, blend[0, 1], numChannels, numSamples
		void operator()(double** samples, double blend,
			int numChannels, int numSamples) noexcept
		{
			if (blend == 0.)
				return;
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto dry = smpls[s];
					const auto wet = noise[rHead];

					smpls[s] = dry + blend * (wet - dry);
					rHead = (rHead + 1) & Max;

					++flipIdx;
					if (flipIdx == FlipLen)
					{
						flipIdx = 0;
						rHead = rand.nextInt(Size);
					}
				}
			}
		}

	protected:
		Random rand;
		std::array<double, Size> noise;
		int rHead, flipIdx;
	};
}