#pragma once
#include "PRM.h"

namespace dsp
{
	struct EnvelopeFollower
	{
		struct Params
		{
			double gainDb, atkMs, dcyMs, smoothMs;
		};

		EnvelopeFollower();

		void prepare(double) noexcept;

		// smpls, params, numSamples
		void operator()(double*, const Params&, int) noexcept;

		// samples, params, numChannels, numSamples
		void operator()(double**, const Params&, int, int) noexcept;

		bool isSleepy() const noexcept;

		double operator[](int i) const noexcept;

		double getMeter() const noexcept;
	private:
		std::atomic<double> meter;
		std::array<double, BlockSize> buffer;
		const double MinDb;
		PRMD gainPRM;
		smooth::LowpassD envLP, smooth;
		double sampleRate, smoothMs;
		bool attackState;

		// samples, numChannels, numSamples
		void copyMid(double**, int, int) noexcept;

		// smpls, numSamples
		void rectify(double*, int) noexcept;

		// gainDb, numSamples
		void applyGain(double, int) noexcept;

		// params, numSamples
		void synthesizeEnvelope(const Params&, int) noexcept;

		// params, s0, s1
		double processAttack(const Params&, double, double) noexcept;

		// params, s0, s1
		double processDecay(const Params&, double, double) noexcept;

		// _smoothMs, numSamples
		void smoothen(double, int) noexcept;

		void processMeter() noexcept;
	};
}
