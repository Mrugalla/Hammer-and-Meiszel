#pragma once
#include "../Using.h"

namespace dsp
{
	struct ResonatorBase
	{
		ResonatorBase();
		
		virtual void reset() noexcept = 0;

		// fc [0, .5]
		void setCutoffFc(double) noexcept;

		// bw [0, .5]
		void setBandwidth(double) noexcept;

		// gain [0, 1]
		void setGain(double) noexcept;

		virtual void update() noexcept = 0;

		virtual double operator()(double) noexcept = 0;

		double fc, bw, gain;
	};

	//////////////////////////////////////////////////////////////////
	// A digital resonator is a recursive (IIR) linear system having a complex conjugate pair of poles located inside the unit circle of the z-plane.
	// The angle of the poles in polar co-ordinates sets the resonant frequency of the resonator, while the distance of the poles are to the unit circle sets the bandwidth.  The closer they are to the unit circle, the smaller the bandwidth.
	// https://www.phon.ucl.ac.uk/courses/spsci/dsp/resoncon.html
	// personal note: incredibly resonant lowpass filter, maybe useful pre-distortion
	struct Resonator1 :
		public ResonatorBase
	{
		Resonator1();

		void reset() noexcept override;

		void update() noexcept override;

		void copyFrom(const Resonator1&) noexcept;

		double operator()(double) noexcept override;

	protected:
		double y1, y2;
		double b1, b2;
	};

	// https://github.com/julianksdj/Resonator2pole/tree/main
	struct Resonator2 :
		public ResonatorBase
	{
		Resonator2();

		void reset() noexcept override;

		void update() noexcept override;

		void copyFrom(const Resonator2&) noexcept;

		double operator()(double) noexcept override;

		double b2, b1, a0;
		double z1, z2;
	};

	template<class ResoClass>
	struct ResonatorStereo
	{
		ResonatorStereo();

		void reset() noexcept;

		// fc [0, .5], ch
		void setCutoffFc(double, int) noexcept;

		// fc [0, .5], ch
		void setBandwidth(double, int) noexcept;

		// gain [0, 1], ch
		void setGain(double, int) noexcept;

		// ch
		void update(int) noexcept;

		void update() noexcept;

		// smpl, ch
		double operator()(double, int) noexcept;

	protected:
		std::array<ResoClass, 2> resonators;
	};

	using ResonatorStereo1 = ResonatorStereo<Resonator1>;
	using ResonatorStereo2 = ResonatorStereo<Resonator2>;
}