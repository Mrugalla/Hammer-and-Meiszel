#pragma once
#include "LadderFilterBase.h"

/*
This class implements Tim Stilson's MoogVCF filter
using 'compromise' poles at z = -0.3

Several improments are built in, such as corrections
for cutoff and resonance parameters, removal of the
necessity of the separation table, audio rate update
of cutoff and resonance and a smoothly saturating
tanh() function, clamping output and creating inherent
nonlinearities.

This code is Unlicensed (i.e. public domain); in an email exchange on
4.21.2018 Aaron Krajeski stated: "That work is under no copyright. 
You may use it however you might like."

Source: http://song-swap.com/MUMT618/aaron/Presentation/demo.html
*/

namespace moog
{
	struct KrajeskiMoog :
		public LadderFilterBase
	{
		KrajeskiMoog()
		{
			memset(state, 0, sizeof(state));
			memset(delay, 0, sizeof(delay));

			drive = 1.;
			gComp = 1.;
		}

		double operator()(double sample) noexcept override
		{
			state[0] = tanh(drive * (sample - 4. * gRes * (state[4] - gComp * sample)));

			for (int i = 0; i < 4; i++)
			{
				state[i + 1] = fclamp(g * (.3 / 1.3 * state[i] + 1. / 1.3 * delay[i] - state[i + 1]) + state[i + 1], -1e30, 1e30);
				delay[i] = state[i];
			}
			return state[4];
		}

		void setResonance(double r) noexcept override
		{
			resonance = r;
			gRes = resonance * (1.0029 + .0526 * wc - .926 * pow(wc, 2.) + 0.0218 * pow(wc, 3.));
		}

		void setCutoff(double c) noexcept override
		{
			cutoff = c;
			wc = MOOG_TAU * cutoff / sampleRate;
			g = 0.9892 * wc - .4342 * pow(wc, 2.) + .1381 * pow(wc, 3.) - .0202 * pow(wc, 4.);
		}

	private:
		double state[5];
		double delay[5];
		double wc; // The angular frequency of the cutoff.
		double g; // A derived parameter for the cutoff frequency
		double gRes; // A similar derived parameter for resonance.
		double gComp; // Compensation factor.
		double drive; // A parameter that controls intensity of nonlinearities.

		inline double fclamp(double in, double min, double max) noexcept
		{
			return fmin(fmax(in, min), max);
		}
	};
}