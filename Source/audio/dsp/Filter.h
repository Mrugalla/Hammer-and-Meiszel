#pragma once
#include <complex>
#include "../Using.h"

namespace dsp
{
	template<typename Float>
	struct FilterBandpass
	{
		/* startVal */
		FilterBandpass(Float = static_cast<Float>(0));

		void clear() noexcept;

		/* frequency fc [0, .5[, q-factor q [1, 160..] */
		void setFc(Float, Float) noexcept;

		void copy(const FilterBandpass<Float>&) noexcept;

		Float operator()(Float) noexcept;

		Float processSample(Float) noexcept;

		/* scaledFreq */
		std::complex<Float> response(Float) const noexcept;
		/* scaledFreq */
		Float responseDb(Float) const noexcept;

	protected:
		Float alpha, cosOmega;
		Float a0, a1, a2, b0, b1, b2;
		Float     x1, x2, y1, y2;

		void updateCoefficients() noexcept;
	};

	using FilterBandpassF = FilterBandpass<float>;
	using FilterBandpassD = FilterBandpass<double>;

	template<size_t NumFilters>
	struct FilterBandpassSlope
	{
		FilterBandpassSlope();

		void clear() noexcept;

		void setStage(int) noexcept;

		/* frequency fc [0, .5[, q-factor q [1, 160..] */
		void setFc(double fc, double q) noexcept;

		void copy(FilterBandpassSlope<NumFilters>&) noexcept;

		double operator()(double) noexcept;

		/* scaledFreq [0, 22050[ */
		std::complex<double> response(double) const noexcept;

	protected:
		std::array<FilterBandpassD, NumFilters> filters;
		int stage;
	};

	//////////////////////////////////////////////////////////////////

	struct IIR
	{
		enum class Type
		{
			LP,
			HP,
			BP,
			BR,
			AP,
			LS,
			HS,
			Notch,
			Bell,
			NumTypes
		};

		/* startVal */
		IIR(double = 0.);

		void clear() noexcept;

		/* type, frequency fc [0, .5[, q-factor q [1, 160..] */
		void setFc(Type, double, double) noexcept;

		/* frequency fc [0, .5[, q-factor q [1, 160..] */
		void setFcBP(double, double) noexcept;

		/* frequency fc [0, .5[, q-factor q [1, 160..] */ // doesn't work yet
		void setFcLP(double, double) noexcept;

		/* frequency fc [0, .5[, q-factor q [1, 160..] */ // doesn't work yet
		void setFcHP(double, double) noexcept;

		void copy(const IIR&) noexcept;

		double operator()(double) noexcept;

		double processSample(double) noexcept;

		/* scaledFreq */
		std::complex<double> response(double) const noexcept;
		/* scaledFreq */
		double responseDb(double) const noexcept;

	protected:
		double alpha, cosOmega;
		double a0, a1, a2, b0, b1, b2;
		double     x1, x2, y1, y2;
	};

	//////////////////////////////////////////////////////////////////


}