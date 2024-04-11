// This file is unlicensed and uncopyright as found at:
// http://www.musicdsp.org/showone.php?id=24
// Considering how widely this same code has been used in ~100 projects on GitHub with 
// various licenses, it might be reasonable to suggest that the license is CC-BY-SA

#pragma once
#include "LadderFilterBase.h"

namespace moog
{
	struct MusicDSPMoog :
		public LadderFilterBase
	{
		MusicDSPMoog() :
			p(0.),
			k(0.),
			t1(0.),
			t2(0.),
			reso(.1)
		{
			memset(stage, 0, sizeof(stage));
			memset(delay, 0, sizeof(delay));
		}

		double operator()(double sample) noexcept override
		{
			const auto x = sample - resonance * stage[3];

			// Four cascaded one-pole filters (bilinear transform)
			stage[0] = x * p + delay[0] * p - k * stage[0];
			stage[1] = stage[0] * p + delay[1] * p - k * stage[1];
			stage[2] = stage[1] * p + delay[2] * p - k * stage[2];
			stage[3] = stage[2] * p + delay[3] * p - k * stage[3];

			// Clipping band-limited sigmoid
			stage[3] -= (stage[3] * stage[3] * stage[3]) / 6.;

			delay[0] = x;
			delay[1] = stage[0];
			delay[2] = stage[1];
			delay[3] = stage[2];

			return stage[3];
		}

		void setResonance(double r) noexcept override
		{
			reso = r;
			resonance = r * (t2 + 6. * t1) / (t2 - 6. * t1);
		}

		void setCutoff(double c) noexcept override
		{
			cutoff = 2. * c / sampleRate;

			p = cutoff * (1.8 - .8 * cutoff);
			k = 2. * sin(cutoff * MOOG_PI * .5) - 1.;
			t1 = (1. - p) * 1.386249;
			t2 = 12. + t1 * t1;

			setResonance(reso);
		}

	private:
		double stage[4];
		double delay[4];
		double p;
		double k;
		double t1;
		double t2;
		double reso;
	};
}

