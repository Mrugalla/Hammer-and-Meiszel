// Based on an implementation by Magnus Jonsson
// https://github.com/magnusjonsson/microtracker (unlicense)

#pragma once
#include "LadderFilterBase.h"

namespace moog
{
	struct MicrotrackerMoog :
		public LadderFilterBase
	{
		MicrotrackerMoog() :
			p0(0.),
			p1(0.),
			p2(0.),
			p3(0.),
			p32(0.),
			p33(0.),
			p34(0.),
			k(resonance * 4.)
		{
		}

		double operator()(double sample) noexcept override
		{
			// Coefficients optimized using differential evolution
			// to make feedback gain 4. correspond closely to the
			// border of instability, for all values of omega.
			auto y = p3 * .360891 + p32 * .417290 + p33 * .177896 + p34 * .0439725;

			p34 = p33;
			p33 = p32;
			p32 = p3;

			p0 += (fast_tanh(sample - k * y) - fast_tanh(p0)) * cutoff;
			p1 += (fast_tanh(p0) - fast_tanh(p1)) * cutoff;
			p2 += (fast_tanh(p1) - fast_tanh(p2)) * cutoff;
			p3 += (fast_tanh(p2) - fast_tanh(p3)) * cutoff;

			return y;
		}

		void setResonance(double r) noexcept override
		{
			resonance = r;
			k = resonance * 4.;
		}

		void setCutoff(double c) noexcept override
		{
			cutoff = c * MOOG_TAU / sampleRate;
			cutoff = moog_min(cutoff, 1.);
		}

	private:
		double p0;
		double p1;
		double p2;
		double p3;
		double p32;
		double p33;
		double p34;
		double k;
	};
}
