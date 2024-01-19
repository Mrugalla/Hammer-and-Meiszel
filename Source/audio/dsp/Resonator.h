#pragma once
#include "../Using.h"

namespace dsp
{
	struct ResonatorBase
	{
		ResonatorBase() :
			fc(0.), bw(0.), gain(1.)
		{}
		
		virtual void reset() noexcept = 0;

		/* fc [0, .5] */
		void setCutoffFc(double _fc) noexcept
		{
			fc = _fc;
		}

		/* bw [0, .5] */
		void setBandwidth(double _bw) noexcept
		{
			bw = _bw;
		}

		/* gain [0, 1] */
		void setGain(double _gain) noexcept
		{
			gain = _gain;
		}

		virtual void update() noexcept = 0;

		virtual double operator()(double x) noexcept = 0;

		double fc, bw, gain;
	};

	//////////////////////////////////////////////////////////////////
	// A digital resonator is a recursive (IIR) linear system having a complex conjugate pair of poles located inside the unit circle of the z-plane.
	// The angle of the poles in polar co-ordinates sets the resonant frequency of the resonator, while the distance of the poles are to the unit circle sets the bandwidth.  The closer they are to the unit circle, the smaller the bandwidth.
	// https://www.phon.ucl.ac.uk/courses/spsci/dsp/resoncon.html
	// personal note: incredibly resonant lowpass filter, maybe useful pre-distortion
	struct Resonator :
		public ResonatorBase
	{
		Resonator() :
			ResonatorBase(),
			y1(0.), y2(0.),
			b1(0.), b2(0.)
		{
			//g = (1. - b2) * std::sin(w);
		}

		void reset() noexcept override
		{
			y1 = 0.;
			y2 = 0.;
		}

		void update() noexcept override
		{
			const auto w = fc * Tau;
			const auto r = 1. - std::sin(Pi * bw);
			b1 = -2. * r * std::cos(w);
			b2 = r * r;
		}

		void copyFrom(const Resonator& other) noexcept
		{
			b1 = other.b1;
			b2 = other.b2;
		}

		double operator()(double x0) noexcept override
		{
			const auto y0 =
				x0
				- b1 * y1
				- b2 * y2;
			y2 = y1;
			y1 = y0;
			return y0 * gain;
		}

	protected:
		double y1, y2;
		double b1, b2;
	};

	// https://github.com/julianksdj/Resonator2pole/tree/main
	struct Resonator2 :
		public ResonatorBase
	{
		Resonator2() :
			ResonatorBase(),
			b2(0.), b1(0.), a0(0.),
			z1(0.), z2(0.)
		{}

		void reset() noexcept override
		{
			z1 = 0.;
			z2 = 0.;
		}

		void update() noexcept override
		{
			const auto freq = Tau * fc;
			b2 = std::exp(-Tau * bw);
			b1 = (-4. * b2 / (1. + b2)) * std::cos(freq);
			a0 = (1. - b2) * std::sqrt(1. - b1 * b1 / (4. * b2));
		}

		void copyFrom(const Resonator2& other) noexcept
		{
			b2 = other.b2;
			b1 = other.b1;
			a0 = other.a0;
		}

		double operator()(double x) noexcept override
		{
			auto y =
				  a0 * x
				- b1 * z1
				- b2 * z2;
			z2 = z1;
			z1 = y;
			return y * gain;
		}

		double b2, b1, a0;
		double z1, z2;
	};

	template<class ResoClass>
	struct ResonatorStereo
	{
		ResonatorStereo() :
			resonators{ ResoClass(), ResoClass() }
		{}

		void reset() noexcept
		{
			resonators[0].reset();
			resonators[1].reset();
		}

		void setCutoffFc(double _fc) noexcept
		{
			for(auto& reso : resonators)
				reso.setCutoffFc(_fc);
		}

		void setBandwidth(double _bw) noexcept
		{
			for(auto& reso : resonators)
				reso.setBandwidth(_bw);
		}

		void setGain(double _gain) noexcept
		{
			for(auto& reso : resonators)
				reso.setGain(_gain);
		}

		void update() noexcept
		{
			resonators[0].update();
			resonators[1].copyFrom(resonators[0]);
		}

		double operator()(double x, int ch) noexcept
		{
			return resonators[ch](x);
		}

	protected:
		std::array<ResoClass, 2> resonators;
	};
}