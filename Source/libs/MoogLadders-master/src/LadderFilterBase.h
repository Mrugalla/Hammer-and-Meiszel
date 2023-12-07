#pragma once
#include "Util.h"

namespace moog
{
	struct LadderFilterBase
	{
		LadderFilterBase() :
			cutoff(1000.),
			resonance(.1),
			sampleRate(1.)
		{}

		void prepare(double _sampleRate) noexcept
		{
			sampleRate = _sampleRate;
		}

		void operator()(double* smpls, int numSamples) noexcept
		{
			for(auto i = 0; i < numSamples; ++i)
				smpls[i] = operator()(smpls[i]);
		}

		virtual double operator()(double) noexcept = 0;
		virtual void setResonance(double r) noexcept = 0;
		virtual void setCutoff(double c) noexcept = 0;

		double getResonance() noexcept
		{
			return resonance;
		}
		double getCutoff() noexcept
		{
			return cutoff;
		}

	protected:
		double cutoff;
		double resonance;
		double sampleRate;
	};
}