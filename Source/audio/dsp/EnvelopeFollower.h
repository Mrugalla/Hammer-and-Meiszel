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

		EnvelopeFollower() :
			buffer(),
			gainPRM(1.),
			envLP(0.),
			smooth(0.),
			sampleRate(1.),
			smoothMs(-1.),
			attackState(false)
		{}

		void prepare(double Fs) noexcept
		{
			sampleRate = Fs;
			gainPRM.prepare(sampleRate, 4.);
			smoothMs = -1.;
			envLP.reset();
		}

		void operator()(double* smpls, Params& params, int numSamples) noexcept
		{
			rectify(smpls, numSamples);
			applyGain(params.gainDb, numSamples);
			synthesizeEnvelope(params, numSamples);
			smoothen(params.smoothMs, numSamples);
		}

	private:
		std::array<double, BlockSize> buffer;
		PRMD gainPRM;
		smooth::LowpassD envLP;
		smooth::SmoothD smooth;
		double sampleRate, smoothMs;
		bool attackState;

		void rectify(double* smpls, int numSamples) noexcept
		{
			for(auto s = 0; s < numSamples; ++s)
				buffer[s] = std::abs(smpls[s]);
		}

		void applyGain(double gainDb, int numSamples) noexcept
		{
			const auto gain = math::dbToAmp(gainDb);
			const auto gainInfo = gainPRM(gain, numSamples);
			if (gainInfo.smoothing)
				return SIMD::multiply(buffer.data(), gainInfo.buf, numSamples);
			SIMD::multiply(buffer.data(), gain, numSamples);
		}

		void synthesizeEnvelope(Params& params, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto s0 = envLP.y1;
				const auto s1 = buffer[s];
				if (attackState)
					buffer[s] = processAttack(params, s0, s1);
				else
					buffer[s] = processDecay(params, s0, s1);
			}
		}

		double processAttack(Params& params, double s0, double s1) noexcept
		{
			if (s0 < s1)
				return envLP(s1);
			attackState = false;
			envLP.makeFromDecayInMs(params.dcyMs, sampleRate);
			return processDecay(params, s0, s1);
		}

		double processDecay(Params& params, double s0, double s1) noexcept
		{
			if (s0 > s1)
				return envLP(s1);
			attackState = true;
			envLP.makeFromDecayInMs(params.atkMs, sampleRate);
			return processAttack(params, s0, s1);
		}

		void smoothen(double _smoothMs, int numSamples) noexcept
		{
			if (smoothMs != _smoothMs)
			{
				smoothMs = _smoothMs;
				smooth.makeFromDecayInMs(smoothMs, sampleRate);
			}
			smooth(buffer.data(), numSamples);
		}
	};
}
