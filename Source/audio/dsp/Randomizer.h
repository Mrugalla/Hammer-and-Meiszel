#pragma once
#include "Perlin.h"

namespace dsp
{
	struct Randomizer
	{
		struct Params
		{
			// gain[0,1], rateSync != 0, smooth[0,1], complex[1,8], dropout[0,1]
			double rateSync, smooth, complex, dropout;
		};

		Randomizer() :
			meter(0.f),
			buffer(),
			perlin()
		{}

		void prepare(double sampleRate) noexcept
		{
			meter.store(0.f);
			perlin.prepare(sampleRate);
		}

		void operator()(const Params& params, const Transport::Info& transport, int numSamples) noexcept
		{
			SIMD::clear(buffer.data(), numSamples);

			perlin
			(
				buffer.data(),
				numSamples, transport,
				1. / params.rateSync,
				params.complex,
				params.dropout,
				params.smooth * params.smooth
			);

			meter.store(static_cast<float>(buffer[numSamples - 1]));
		}

		double operator[](int i) const noexcept
		{
			return buffer[i];
		}

		float getMeter() const noexcept
		{
			return meter.load();
		}
	private:
		std::atomic<float> meter;
		std::array<double, BlockSize> buffer;
		perlin::Perlin2 perlin;
	};
}