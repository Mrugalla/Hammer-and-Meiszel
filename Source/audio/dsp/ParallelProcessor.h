#pragma once
#include "../Using.h"

namespace dsp
{
	template<size_t NumBands>
	struct ParallelProcessor
	{
		static constexpr int MaxBand = NumBands - 1;
		static constexpr int NumChannels = 2 * MaxBand;

		struct Band
		{
			double* l;
			double* r;
		};

		ParallelProcessor();

		void split(double* const*, int, int) noexcept;

		void join(double* const*, int, int) noexcept;

		/* gain, bandIdx, numChannels, numSamples */
		void applyGain(double, int, int, int) noexcept;

		/* gain, bandIdx, numChannels, numSamples */
		void applyGain(double*, int, int, int) noexcept;

		Band getBand(int) noexcept;

		/* only for parallel processors with exactly 2 bands
		samples, mix, numChannels, numSamples */
		void joinMix(double* const*, double*, int, int) noexcept;

		/* only for parallel processors with exactly 2 bands
		samples, mix, numChannels, numSamples */
		void joinMix(double* const*, double, int, int) noexcept;

		/* only for parallel processors with exactly 2 bands
		samples, gain, numChannels, numSamples */
		void joinDelta(double* const*, double*, int, int) noexcept;

		/* only for parallel processors with exactly 2 bands
		samples, gain, numChannels, numSamples */
		void joinDelta(double* const*, double, int, int) noexcept;

	private:
		std::array<std::array<double, BlockSize>, NumChannels> bands;
	};

	using PP2Band = ParallelProcessor<2>;
	using PP3Band = ParallelProcessor<3>;
	using PP4Band = ParallelProcessor<4>;
	using PP5Band = ParallelProcessor<5>;
}