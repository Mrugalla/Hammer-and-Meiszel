#pragma once
#include "Transport.h"
#include "Phasor.h"
#include "Smooth.h"
#include <random>

namespace dsp
{
	class Randomizer
	{
		struct RandValPool
		{
			static constexpr int Order = 9;
			static constexpr int Size = 1 << Order;
			static constexpr int Max = Size - 1;

			RandValPool() :
				rd(),
				gen(rd()),
				dist(0., 1.),
				pool()
			{
				gen.seed(420);
				for (auto i = 0; i < Size; ++i)
					pool[i] = dist(gen);
			}

			double operator()(int pos) const noexcept
			{
				return pool[pos & Max];
			}
		private:
			std::random_device rd;
			std::mt19937 gen;
			std::uniform_real_distribution<double> dist;
			std::array<double, Size> pool;
		};
	public:
		struct Params
		{
			// gain[0,1], rateSync != 0, smooth[0,1], spread[0,1]
			double gain, rateSync, smooth, spread;
		};

		Randomizer() :
			randPool(),
			meter(0.f),
			buffer(),
			phasor(),
			rateSync(1. / 16.f), phase(0.),
			val0(0.), val1(0.), valRange(1.),
			sampleRate(1.),
			sampleRateInv(1.)
		{}

		void prepare(double _sampleRate) noexcept
		{
			sampleRate = _sampleRate;
			sampleRateInv = 1. / sampleRate;
			phasor.prepare(sampleRateInv);
			meter.store(0.f);
		}

		void operator()(const Params& params, const Transport::Info& transport, int numSamples) noexcept
		{
			const auto phaseInfo = transport.getPhaseInfo(rateSync, sampleRateInv);
			const auto posFloor = std::floor(phaseInfo.phase);
			const auto posInt = static_cast<int>(posFloor);
			phasor.phase.phase = phaseInfo.phase - posFloor;
			phasor.inc = phaseInfo.inc;

			if (params.gain == 0.f)
			{
				SIMD::clear(buffer.data(), numSamples);
				meter.store(0.f);
				return;
			}

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto curPhase = phasor.phase.phase;
				phase = phasor().phase;
				const auto distance = phase - curPhase;
				if (distance < 0.)
				{
					if (rateSync != params.rateSync)
						rateSync = params.rateSync;

					const auto nextVal = randPool(posInt);

					val0 = val1;
					val1 = val0 + params.spread * (nextVal - val0);
					valRange = val1 - val0;
				}
				const auto phaseSquared = curPhase * curPhase;
				const auto y = val0 + params.smooth * phaseSquared * valRange;
				buffer[s] = y;
				phase = curPhase;
			}

			if(params.gain != 1.f)
				SIMD::multiply(buffer.data(), params.gain, numSamples);

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
		RandValPool randPool;
		std::atomic<float> meter;
		std::array<double, BlockSize> buffer;
		PhasorD phasor;
		double rateSync, phase, val0, val1, valRange, sampleRate, sampleRateInv;
	};
}