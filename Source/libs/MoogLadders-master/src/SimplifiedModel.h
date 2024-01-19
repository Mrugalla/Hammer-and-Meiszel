/* -------------------------------------------------------------------------
 * This source code is provided without any warranties as published in 
 * DAFX book 2nd edition, copyright Wiley & Sons 2011, available at 
 * http://www.dafx.de. It may be used for educational purposes and not 
 * for commercial applications without further permission.
 * -------------------------------------------------------------------------
 */

#pragma once
#include "LadderFilterBase.h"

/*
The simplified nonlinear Moog filter is based on the full Huovilainen model,
with five nonlinear (tanh) functions (4 first-order sections and a feedback).
Like the original, this model needs an oversampling factor of at least two when
these nonlinear functions are used to reduce possible aliasing. This model
maintains the ability to self oscillate when the feedback gain is >= 1.0.

References: DAFX - Zolzer (ed) (2nd ed)
Original implementation: Valimaki, Bilbao, Smith, Abel, Pakarinen, Berners (DAFX)
This is a transliteration into C++ of the original matlab source (moogvcf.m)

Considerations for oversampling:
http://music.columbia.edu/pipermail/music-dsp/2005-February/062778.html
http://www.synthmaker.co.uk/dokuwiki/doku.php?id=tutorials:oversampling
*/

namespace moog
{
	struct SimplifiedMoog :
		public LadderFilterBase
	{
		// To keep the overall level approximately constant, comp should be set
		// to 0.5 resulting in a 6 dB passband gain decrease at the maximum resonance
		// (compared to a 12 dB decrease in the original Moog model
		SimplifiedMoog() :
			y(0.),
			h(0.), h0(0.), g(0.), g1(0.)
		{
			memset(stage, 0, sizeof(stage));
			memset(stageZ1, 0, sizeof(stageZ1));
			memset(stageTanh, 0, sizeof(stageTanh));
		}

		void copyFrom(SimplifiedMoog& other) noexcept
		{
			h = other.h;
			h0 = other.h0;
			g = other.g;
			g1 = other.g1;
		}

		// This system is nonlinear so we are probably going to create a signal with components that exceed nyquist.
		// To prevent aliasing distortion, we need to oversample this processing chunk. Where do these extra samples
		// come from? Todo! We can use polynomial interpolation to generate the extra samples, but this is expensive.
		// The cheap solution is to zero-stuff the incoming sample buffer.
		// With resampling, numSamples should be 2x the frame size of the existing sample rate.
		// The output of this filter needs to be run through a decimator to return to the original samplerate.
		double operator()(double x) noexcept override
		{
			auto input = x - (resonance * (y - x));
			stage[0] = (h * tanh(input) + h0 * stageZ1[0]) + g1 * stageTanh[0];
			stageZ1[0] = stage[0];

			for (auto stageIdx = 1; stageIdx < 3; ++stageIdx)
			{
				input = stage[stageIdx - 1];
				stageTanh[stageIdx - 1] = tanh(input);
				stage[stageIdx] = h * stageZ1[stageIdx] + h0 * stageTanh[stageIdx - 1] + g1 * stageTanh[stageIdx];
				stageZ1[stageIdx] = stage[stageIdx];
			}

			input = stage[2];
			stageTanh[2] = tanh(input);
			stage[3] = h * stageZ1[3] + h0 * stageTanh[2] + g1 * tanh(stageZ1[3]);
			stageZ1[3] = stage[3];

			y = SNAP_TO_ZERO(stage[3]);
			return y;
		}

		void setResonance(double r) noexcept override
		{
			resonance = 4. * r;
		}

		void setCutoff(double c) noexcept override
		{
			cutoff = c;

			// Normalized cutoff [0, 1] in radians
			g = MOOG_TAU * cutoff / sampleRate;
			g *= MOOG_PI / 1.3; // correction factor that allows _cutoff to be supplied Hertz
			g1 = 1. - g;

			// FIR part with gain g
			h = g / 1.3;
			h0 = g * .3 / 1.3;
		}

	private:
		double y;
		double stage[4];
		double stageZ1[4];
		double stageTanh[3];
		double h, h0, g, g1;
	};
}
