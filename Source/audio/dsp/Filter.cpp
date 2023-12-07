#include "Filter.h"
#include "../Using.h"

namespace dsp
{
	template<typename Float>
	FilterBandpass<Float>::FilterBandpass(Float startVal) :
		alpha(static_cast<Float>(0)),
		cosOmega(static_cast<Float>(0)),
		a0(static_cast<Float>(0)),
		a1(static_cast<Float>(0)),
		a2(static_cast<Float>(0)),
		b0(static_cast<Float>(0)),
		b1(static_cast<Float>(0)),
		b2(static_cast<Float>(0)),
		x1(static_cast<Float>(0)),
		x2(static_cast<Float>(0)),
		y1(startVal),
		y2(startVal)
	{

	}

	template<typename Float>
	void FilterBandpass<Float>::clear() noexcept
	{
		x1 = 0.f;
		x2 = 0.f;
		y1 = 0.f;
		y2 = 0.f;
	}

	template<typename Float>
	void FilterBandpass<Float>::setFc(Float fc, Float q) noexcept
	{
		const auto omega = static_cast<Float>(Tau) * fc;
		const auto two = static_cast<Float>(2);
		cosOmega = -two * std::cos(omega);
		const auto sinOmega = std::sin(omega);
		alpha = sinOmega / (two * q);

		updateCoefficients();
	}

	template<typename Float>
	void FilterBandpass<Float>::copy(const FilterBandpass& other) noexcept
	{
		a0 = other.a0;
		a1 = other.a1;
		a2 = other.a2;
		b0 = other.b0;
		b1 = other.b1;
		b2 = other.b2;
	}

	template<typename Float>
	Float FilterBandpass<Float>::operator()(Float x0) noexcept
	{
		return processSample(x0);
	}

	template<typename Float>
	Float FilterBandpass<Float>::processSample(Float x0) noexcept
	{
		auto y0 =
			x0 * a0 +
			x1 * a1 +
			x2 * a2 -
			y1 * b1 -
			y2 * b2;

		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = y0;

		return y0;
	}

	template<typename Float>
	void FilterBandpass<Float>::updateCoefficients() noexcept
	{
		const auto one = static_cast<Float>(1);

		a0 = alpha;
		a1 = static_cast<Float>(0);
		a2 = -alpha;

		b1 = cosOmega;
		b2 = one - alpha;

		b0 = one + alpha;
	}

	template<typename Float>
	std::complex<Float> FilterBandpass<Float>::response(Float scaledFreq) const noexcept
	{
		const auto one = static_cast<Float>(1);
		const auto w = scaledFreq * static_cast<Float>(Tau);
		std::complex<Float> z = { std::cos(w), -std::sin(w) };
		std::complex<Float> z2 = z * z;

		//return (b0 + z * b1 + z2 * b2) / (one + z * a1 + z2 * a2);
		return (a0 + z * a1 + z2 * a2) / (one + z * b1 + z2 * b2);
	}

	template<typename Float>
	Float FilterBandpass<Float>::responseDb(Float scaledFreq) const noexcept
	{
		const auto one = static_cast<Float>(1);
		const auto ten = static_cast<Float>(10);
		const auto w = scaledFreq * static_cast<Float>(Tau);
		std::complex<Float> z = { std::cos(w), -std::sin(w) };
		std::complex<Float> z2 = z * z;
		const auto energy = std::norm(a0 + z * a1 + z2 * a2) / std::norm(one + z * b1 + z2 * b2);
		return ten * std::log10(energy);
	}

	template struct FilterBandpass<float>;
	template struct FilterBandpass<double>;

	// FilterBandpassSlope

	template<size_t NumFilters>
	FilterBandpassSlope<NumFilters>::FilterBandpassSlope() :
		filters(),
		stage(1)
	{}

	template<size_t NumFilters>
	void FilterBandpassSlope<NumFilters>::clear() noexcept
	{
		for (auto& filter : filters)
			filter.clear();
	}

	template<size_t NumFilters>
	void FilterBandpassSlope<NumFilters>::setStage(int s) noexcept
	{
		stage = s;
	}

	template<size_t NumFilters>
	void FilterBandpassSlope<NumFilters>::setFc(double fc, double q) noexcept
	{
		filters[0].setFc(fc, q);
		for (auto i = 1; i < stage; ++i)
			filters[i].copy(filters[0]);
	}

	template<size_t NumFilters>
	void FilterBandpassSlope<NumFilters>::copy(FilterBandpassSlope<NumFilters>& other) noexcept
	{
		for (auto i = 0; i < stage; ++i)
			filters[i].copy(other.filters[i]);
		stage = other.stage;
	}

	template<size_t NumFilters>
	double FilterBandpassSlope<NumFilters>::operator()(double x) noexcept
	{
		for (auto i = 0; i < stage; ++i)
			x = filters[i](x);
		return x;
	}

	template<size_t NumFilters>
	std::complex<double> FilterBandpassSlope<NumFilters>::response(double scaledFreq) const noexcept
	{
		std::complex<double> response = { 1., 0. };
		for (auto i = 0; i < stage; ++i)
			response *= filters[i].response(scaledFreq);
		return response;
	}

	template struct FilterBandpassSlope<1>;
	template struct FilterBandpassSlope<2>;
	template struct FilterBandpassSlope<3>;
	template struct FilterBandpassSlope<4>;
	template struct FilterBandpassSlope<5>;
	template struct FilterBandpassSlope<6>;
	template struct FilterBandpassSlope<7>;
	template struct FilterBandpassSlope<8>;

	////////////////////////////////////////////////////////////////////

	// IIR

	IIR::IIR(double startVal) :
		alpha(0.),
		cosOmega(0.),
		a0(0.),
		a1(0.),
		a2(0.),
		b0(0.),
		b1(0.),
		b2(0.),
		x1(0.),
		x2(0.),
		y1(startVal),
		y2(startVal)
	{
	}

	void IIR::clear() noexcept
	{
		x1 = 0.;
		x2 = 0.;
		y1 = 0.;
		y2 = 0.;
	}

	void IIR::setFc(Type type, double fc, double q) noexcept
	{
		switch (type)
		{
		case Type::LP: return setFcLP(fc, q);
		case Type::HP: return setFcHP(fc, q);
		case Type::BP: return setFcBP(fc, q);
		case Type::BR: return;
		case Type::AP: return;
		case Type::LS: return;
		case Type::HS: return;
		case Type::Notch: return;
		default: return; // type == Type::Bell
		}
	}

	void IIR::setFcBP(double fc, double q) noexcept
	{
		const auto omega = Tau * fc;
		cosOmega = -2. * std::cos(omega);
		const auto sinOmega = std::sin(omega);
		alpha = sinOmega / (2. * q);
		const auto scale = 1. + (q / 2.);

		a0 = alpha * scale;
		a1 = 0.;
		a2 = -alpha * scale;

		b1 = cosOmega;
		b2 = 1. - alpha;

		b0 = 1. + alpha;
		const auto b0Inv = 1. / b0;

		a0 *= b0Inv;
		a1 *= b0Inv;
		a2 *= b0Inv;
		b1 *= b0Inv;
		b2 *= b0Inv;
	}

	void IIR::setFcLP(double, double) noexcept
	{

	}

	void IIR::setFcHP(double fc, double q) noexcept
	{
		const auto omega = Tau * fc;
		cosOmega = -2. * std::cos(omega);
		const auto sinOmega = std::sin(omega);
		alpha = sinOmega / (2. * q);
		const auto scale = 1. + (q / 2.);

		a0 = (1. + alpha) * scale;
		a1 = -2. * cosOmega * scale;
		a2 = (1. - alpha) * scale;

		b1 = -2. * cosOmega;
		b2 = 1. - alpha;

		b0 = 1. + alpha;
		const auto b0Inv = 1. / b0;

		a0 *= b0Inv;
		a1 *= b0Inv;
		a2 *= b0Inv;
		b1 *= b0Inv;
		b2 *= b0Inv;
	}

	void IIR::copy(const IIR& other) noexcept
	{
		a0 = other.a0;
		a1 = other.a1;
		a2 = other.a2;
		b0 = other.b0;
		b1 = other.b1;
		b2 = other.b2;
	}

	double IIR::operator()(double x0) noexcept
	{
		return processSample(x0);
	}

	double IIR::processSample(double x0) noexcept
	{
		auto y0 =
			x0 * a0 +
			x1 * a1 +
			x2 * a2 -
			y1 * b1 -
			y2 * b2;

		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = y0;

		return y0;
	}

	std::complex<double> IIR::response(double scaledFreq) const noexcept
	{
		const auto w = scaledFreq * Tau;
		std::complex<double> z = { std::cos(w), -std::sin(w) };
		std::complex<double> z2 = z * z;

		return (a0 + z * a1 + z2 * a2) / (1. + z * b1 + z2 * b2);
	}

	double IIR::responseDb(double scaledFreq) const noexcept
	{
		const auto w = scaledFreq * Tau;
		std::complex<double> z = { std::cos(w), -std::sin(w) };
		std::complex<double> z2 = z * z;
		const auto energy = std::norm(a0 + z * a1 + z2 * a2) / std::norm(1. + z * b1 + z2 * b2);
		return 10. * std::log10(energy);
	}
}