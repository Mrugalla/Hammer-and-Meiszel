#include "ParallelProcessor.h"

namespace dsp
{
	template<size_t NumBands>
	ParallelProcessor<NumBands>::ParallelProcessor() :
		bands()
	{}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::split(double* const* samples, int numChannels, int numSamples) noexcept
	{
		for (auto b = 0; b < MaxBand; ++b)
		{
			const auto b2 = 2 * b;
			double* band[] = { bands[b2].data(), bands[b2 + 1].data() };

			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::copy(band[ch], samples[ch], numSamples);
		}
	}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::join(double* const* samples, int numChannels, int numSamples) noexcept
	{
		for (auto b = 0; b < MaxBand; ++b)
		{
			const auto b2 = 2 * b;
			const double* band[] = { bands[b2].data(), bands[b2 + 1].data() };

			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::add(samples[ch], band[ch], numSamples);
		}
	}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::applyGain(double gain, int bandIdx, int numChannels, int numSamples) noexcept
	{
		const auto b2 = 2 * bandIdx;
		double* band[] = { bands[b2].data(), bands[b2 + 1].data() };

		for (auto ch = 0; ch < numChannels; ++ch)
			SIMD::multiply(band[ch], gain, numSamples);
	}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::applyGain(double* gain, int bandIdx, int numChannels, int numSamples) noexcept
	{
		const auto b2 = 2 * bandIdx;
		double* band[] = { bands[b2].data(), bands[b2 + 1].data() };

		for (auto ch = 0; ch < numChannels; ++ch)
			SIMD::multiply(band[ch], gain, numSamples);
	}

	template<size_t NumBands>
	ParallelProcessor<NumBands>::Band ParallelProcessor<NumBands>::getBand(int bandIdx) noexcept
	{
		const auto b2 = 2 * bandIdx;
		return { bands[b2].data(), bands[b2 + 1].data() };
	}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::joinMix(double* const* samples, double* mix,
		int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto band = bands[ch].data();

			for (auto s = 0; s < numSamples; ++s)
				smpls[s] = band[s] + mix[s] * (smpls[s] - band[s]);
		}
	}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::joinMix(double* const* samples, double mix,
		int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto band = bands[ch].data();

			for (auto s = 0; s < numSamples; ++s)
				smpls[s] = band[s] + mix * (smpls[s] - band[s]);
		}
	}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::joinDelta(double* const* samples, double* gain,
		int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto band = bands[ch].data();

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto g = gain[s];
				const auto dry = band[s];
				const auto wet = smpls[s];
				const auto delta = wet - dry;

				smpls[s] = g * delta;
			}
		}
	}

	template<size_t NumBands>
	void ParallelProcessor<NumBands>::joinDelta(double* const* samples, double gain,
		int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto band = bands[ch].data();

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dry = band[s];
				const auto wet = smpls[s];
				const auto delta = wet - dry;

				smpls[s] = gain * delta;
			}
		}
	}

	template struct ParallelProcessor<2>;
	template struct ParallelProcessor<3>;
	template struct ParallelProcessor<4>;
	template struct ParallelProcessor<5>;
}