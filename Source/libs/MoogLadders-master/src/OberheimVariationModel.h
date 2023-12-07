// See: http://www.willpirkle.com/forum/licensing-and-book-code/licensing-and-using-book-code/
// The license is "You may also use the code from the FX and Synth books without licensing or fees. 
// The code is for you to develop your own plugins for your own use or for commercial use."

#pragma once
#include "LadderFilterBase.h"

namespace moog
{
	struct VAOnePole
	{
		VAOnePole()
		{
			reset();
		}

		void reset() noexcept
		{
			alpha = 1.;
			beta = 0.;
			gamma = 1.;
			delta = 0.;
			epsilon = 0.;
			a0 = 1.;
			feedback = 0.;
			z1 = 0.;
		}

		double tick(double s) noexcept
		{
			s = s * gamma + feedback + epsilon * getFeedbackOutput();
			const auto vn = (a0 * s - z1) * alpha;
			auto out = vn + z1;
			z1 = vn + out;
			return out;
		}

		void setFeedback(double fb) noexcept { feedback = fb; }
		double getFeedbackOutput() noexcept { return beta * (z1 + feedback * delta); }
		void setAlpha(double a) noexcept { alpha = a; };
		void setBeta(double b) noexcept { beta = b; };

	private:
		double alpha, beta, gamma, delta;
		double epsilon, a0, feedback, z1;
	};

	struct OberheimVariationMoog :
		public LadderFilterBase
	{
		OberheimVariationMoog() :
			saturation(1.),
			Q(3.)
		{
		}

		double operator()(double sample) noexcept override
		{
			auto input = sample;

			double sigma =
				LPF1.getFeedbackOutput() +
				LPF2.getFeedbackOutput() +
				LPF3.getFeedbackOutput() +
				LPF4.getFeedbackOutput();

			input *= 1. + K;

			// calculate input to first filter
			auto u = (input - K * sigma) * alpha0;

			u = tanh(saturation * u);

			double stage1 = LPF1.tick(u);
			double stage2 = LPF2.tick(stage1);
			double stage3 = LPF3.tick(stage2);
			double stage4 = LPF4.tick(stage3);

			// Oberheim variations
			return oberheimCoefs[0] * u +
				oberheimCoefs[1] * stage1 +
				oberheimCoefs[2] * stage2 +
				oberheimCoefs[3] * stage3 +
				oberheimCoefs[4] * stage4;
		}

		void setResonance(double r) noexcept override
		{
			// this maps resonance = 1->10 to K = 0 -> 4
			K = 4. * (r - 1.) / (10. - 1.);
		}

		void setCutoff(double c) noexcept override
		{
			cutoff = c;

			// prewarp for BZT
			double wd = MOOG_TAU * cutoff;
			double T = 1. / sampleRate;
			double wa = (2. / T) * tan(wd * T / 2.);
			double g = wa * T / 2.;

			// Feedforward coeff
			double G = g / (1.0 + g);

			LPF1.setAlpha(G);
			LPF2.setAlpha(G);
			LPF3.setAlpha(G);
			LPF4.setAlpha(G);

			LPF1.setBeta(G * G * G / (1. + g));
			LPF2.setBeta(G * G / (1. + g));
			LPF3.setBeta(G / (1. + g));
			LPF4.setBeta(1. / (1. + g));

			gamma = G * G * G * G;
			alpha0 = 1. / (1. + K * gamma);

			// Oberheim variations / LPF4
			oberheimCoefs[0] = 0.;
			oberheimCoefs[1] = 0.;
			oberheimCoefs[2] = 0.;
			oberheimCoefs[3] = 0.;
			oberheimCoefs[4] = 1.;
		}

	private:

		VAOnePole LPF1, LPF2, LPF3, LPF4;
		double K, gamma, alpha0, Q, saturation;
		double oberheimCoefs[5];
	};
}

