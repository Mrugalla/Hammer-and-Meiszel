#pragma once

#include <stdint.h>
#include <array>
#include "Util.h"

namespace moog
{
	struct BiquadFilter
	{
		enum Type { LP, HP, BP, AP, NOTCH, PEAK, LS, HS };
		static constexpr double DefaultCutoffFc = 1000. / 44100.;
		static constexpr double DefaultResonance = .8;

		BiquadFilter() :
			type(Type::BP),
			bCoef{ 0., 0., 0. },
			aCoef{ 0., 0. },
			w{ 0., 0. },
			omega(DefaultCutoffFc),
			cosOmega(0.), sinOmega(0.),
			Q(DefaultResonance), alpha(0.), A(1.),
			a{ 0., 0., 0. },
			b{ 0., 0., 0. }
		{
		}

		void prepare() noexcept
		{
			update();
		}

		void operator()(double* samples, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				processSample(samples[s]);
		}

		double operator()(double sample) noexcept
		{
			return processSample(sample);
		}

		/* fc = hz / sampleRate */
		void setCutoffFc(double fc) noexcept
		{
			omega = MOOG_PI_2 * fc;
		}

		void setResonance(double q) noexcept
		{
			Q = q;
		}

		void setType(Type _type) noexcept
		{
			type = _type;
		}

		void update() noexcept
		{
			cosOmega = cos(omega);
			sinOmega = sin(omega);

			switch (type)
			{
			case LP:
				alpha = sinOmega / (2. * Q);
				b[0] = (1. - cosOmega) / 2.;
				b[1] = 1. - cosOmega;
				b[2] = b[0];
				a[0] = 1. + alpha;
				a[1] = -2. * cosOmega;
				a[2] = 1. - alpha;
				break;
			case HP:
				alpha = sinOmega / (2. * Q);
				b[0] = (1. + cosOmega) / 2.;
				b[1] = -(1. + cosOmega);
				b[2] = b[0];
				a[0] = 1. + alpha;
				a[1] = -2. * cosOmega;
				a[2] = 1. - alpha;
				break;
			case BP:
				alpha = sinOmega * sinh(log(2.) / 2. * Q * omega / sinOmega);
				b[0] = sinOmega / 2.;
				b[1] = 0.;
				b[2] = -b[0];
				a[0] = 1. + alpha;
				a[1] = -2. * cosOmega;
				a[2] = 1. - alpha;
				break;
			case AP:
				alpha = sinOmega / (2. * Q);
				b[0] = 1. - alpha;
				b[1] = -2. * cosOmega;
				b[2] = 1. + alpha;
				a[0] = b[2];
				a[1] = b[1];
				a[2] = b[0];
				break;
			case NOTCH:
				alpha = sinOmega * sinh(log(2.) / 2. * Q * omega / sinOmega);
				b[0] = 1.;
				b[1] = -2. * cosOmega;
				b[2] = 1.;
				a[0] = 1. + alpha;
				a[1] = b[1];
				a[2] = 1. - alpha;
				break;
			case PEAK:
				alpha = sinOmega * sinh(log(2.) / 2. * Q * omega / sinOmega);
				b[0] = 1. + (alpha * A);
				b[1] = -2. * cosOmega;
				b[2] = 1. - (alpha * A);
				a[0] = 1. + (alpha / A);
				a[1] = b[1];
				a[2] = 1. - (alpha / A);
				break;
			case LS:
				alpha = sinOmega / 2. * sqrt((A + 1. / A) * (1. / Q - 1.) + 2.);
				b[0] = A * ((A + 1.) - ((A - 1.) * cosOmega) + (2. * sqrt(A) * alpha));
				b[1] = 2. * A * ((A - 1.) - ((A + 1.) * cosOmega));
				b[2] = A * ((A + 1.) - ((A - 1.) * cosOmega) - (2. * sqrt(A) * alpha));
				a[0] = ((A + 1.) + ((A - 1.) * cosOmega) + (2. * sqrt(A) * alpha));
				a[1] = -2. * ((A - 1.) + ((A + 1.) * cosOmega));
				a[2] = ((A + 1.) + ((A - 1.) * cosOmega) - (2. * sqrt(A) * alpha));
				break;
			case HS:
				alpha = sinOmega / 2. * sqrt((A + 1. / A) * (1. / Q - 1.) + 2.);
				b[0] = A * ((A + 1.) + ((A - 1.) * cosOmega) + (2. * sqrt(A) * alpha));
				b[1] = -2. * A * ((A - 1.) + ((A + 1.) * cosOmega));
				b[2] = A * ((A + 1.) + ((A - 1.) * cosOmega) - (2. * sqrt(A) * alpha));
				a[0] = ((A + 1.) - ((A - 1.) * cosOmega) + (2. * sqrt(A) * alpha));
				a[1] = 2. * ((A - 1.) - ((A + 1.) * cosOmega));
				a[2] = ((A + 1.) - ((A - 1.) * cosOmega) - (2. * sqrt(A) * alpha));
				break;
			}

			// Normalize filter coefficients
			const auto factor = 1. / a[0];

			aCoef[0] = a[1] * factor;
			aCoef[1] = a[2] * factor;

			bCoef[0] = b[0] * factor;
			bCoef[1] = b[1] * factor;
			bCoef[2] = b[2] * factor;
		}

		void copyFrom(const BiquadFilter& other) noexcept
		{
			for(auto i = 0; i < bCoef.size(); ++i)
				bCoef[i] = other.bCoef[i];
			for (auto i = 0; i < aCoef.size(); ++i)
				aCoef[i] = other.aCoef[i];
			for (auto i = 0; i < w.size(); ++i)
				w[i] = other.w[i];
		}

		Type type;

		std::array<double, 3> bCoef; // b0, b1, b2
		std::array<double, 2> aCoef; // a1, a2
		std::array<double, 2> w; // delays

		double omega, cosOmega, sinOmega, Q, alpha, A;
		std::array<double, 3> a;
		std::array<double, 3> b;

		// DF-II impl
		double processSample(double sample) noexcept
		{
			auto y = bCoef[0] * sample + w[0];
			w[0] = bCoef[1] * sample - aCoef[0] * y + w[1];
			w[1] = bCoef[2] * sample - aCoef[1] * y;
			return y;
		}
	};

	template<size_t MaxSlope>
	struct BiquadFilterSlope
	{
		using Filters = std::array<BiquadFilter, MaxSlope>;
		using Type = typename BiquadFilter::Type;

		BiquadFilterSlope() :
			filters(),
			cutoffFc(BiquadFilter::DefaultCutoffFc),
			reso(BiquadFilter::DefaultResonance),
			type(Type::BP),
			slope(MaxSlope)
		{}

		void prepare() noexcept
		{
			for(auto& f: filters)
				f.prepare();
		}

		void operator()(double* samples, int numSamples) noexcept
		{
			for (auto& filter : filters)
				filter(samples, numSamples);
		}

		double operator()(double sample) noexcept
		{
			auto y = filters[0](sample);
			for(auto i = 1; i < slope; ++i)
				y = filters[i](y);
			return y;
		}

		/* fc = hz / sampleRate */
		void setCutoffFc(double fc) noexcept
		{
			cutoffFc = fc;
			filters[0].setCutoffFc(cutoffFc);
			for(auto i = 1; i < slope; ++i)
				filters[i].omega = filters[0].omega;
		}

		void setResonance(double q) noexcept
		{
			reso = q;
			for (auto& filter : filters)
				filter.setResonance(reso);
		}

		void setType(Type _type) noexcept
		{
			type = _type;
			for (auto& filter : filters)
				filter.setType(type);
		}

		void setSlope(int s) noexcept
		{
			slope = s;
		}

		void update() noexcept
		{
			filters[0].update();
			for(auto i = 0; i < slope; ++i)
				filters[i].copyFrom(filters[0]);
		}

		void copyFrom(const BiquadFilterSlope<MaxSlope>& other) noexcept
		{
			for(auto i = 0; i < MaxSlope; ++i)
				filters[i].copyFrom(other.filters[i]);
			cutoffFc = other.cutoffFc;
			reso = other.reso;
			type = other.type;
			slope = other.slope;
		}

	protected:
		Filters filters;
		double cutoffFc, reso;
		Type type;
		int slope;
	};
}

