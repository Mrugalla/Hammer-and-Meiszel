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

		// samples, numChannels, numSamples
		void split(double* const*, int, int) noexcept;

		// samples, numChannels, numSamples
		void join(double* const*, int, int) noexcept;

		// samples, numChannels, numSamples
		void joinReplace(double* const*, int, int) noexcept;

		// gain, bandIdx, numChannels, numSamples
		void applyGain(double, int, int, int) noexcept;

		// gain, bandIdx, numChannels, numSamples
		void applyGain(double*, int, int, int) noexcept;

		Band getBand(int) noexcept;

		Band operator[](int) noexcept;

		// only for parallel processors with exactly 2 bands
		//samples, mix, numChannels, numSamples
		void joinMix(double* const*, double*, int, int) noexcept;

		// only for parallel processors with exactly 2 bands
		//samples, mix, numChannels, numSamples
		void joinMix(double* const*, double, int, int) noexcept;

		// only for parallel processors with exactly 2 bands
		//samples, gain, numChannels, numSamples
		void joinDelta(double* const*, double*, int, int) noexcept;

		//only for parallel processors with exactly 2 bands
		//samples, gain, numChannels, numSamples
		void joinDelta(double* const*, double, int, int) noexcept;

		// bandIdx
		bool isSleepy(int) const noexcept;

		// sleepyState, bandIdx
		void setSleepy(bool, int) noexcept;

	private:
		std::array<std::array<double, BlockSize2x>, NumChannels> bands;
		std::array<bool, NumBands> sleepy;
	};

	using PP2Band = ParallelProcessor<2>;
	using PP3Band = ParallelProcessor<3>;
	using PP4Band = ParallelProcessor<4>;
	using PP5Band = ParallelProcessor<5>;

	using PPMPEBand = ParallelProcessor<NumMPEChannels>;
	using PPMIDIBand = ParallelProcessor<NumMIDIChannels>;
}