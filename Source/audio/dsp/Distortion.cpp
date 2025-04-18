#include "Distortion.h"

namespace dsp
{
	template<typename Float>
	Float hardclip(Float smpl, Float ceiling) noexcept
	{
		return smpl < -ceiling ? -ceiling : smpl > ceiling ? ceiling : smpl;
	}

	template<typename Float>
	Float ratioclip(Float smpl, Float threshold, Float ratioInv) noexcept
	{
		return smpl < threshold ? smpl : ratioInv * (smpl - threshold) + threshold;
	}

	template<typename Float>
	Float softclipCubic(Float smpl, Float ceiling) noexcept
	{
		static constexpr auto three = static_cast<Float>(3);
		static constexpr auto two = static_cast<Float>(2);
		
		return smpl < -ceiling ? -ceiling : smpl > ceiling ? ceiling :
			(smpl - smpl * smpl * smpl / three) * three / two * ceiling;
	}

	template<typename Float>
	Float softclipAtan(Float sample, Float ceiling, Float alpha) noexcept
	{
		static constexpr auto a = static_cast<Float>(2. / Pi);
		return a * ceiling * std::atan(alpha * sample);
	}

	template<typename Float>
	Float softclipSigmoid(Float sample, Float ceiling, Float k) noexcept
	{
		static constexpr auto one = static_cast<Float>(1);
		static constexpr auto two = static_cast<Float>(2);
		static constexpr auto e = static_cast<Float>(2.71828182845904523536028747135266249775724709369995);

		return two * ceiling / (one + std::pow(e, -k * sample)) - ceiling;
	}

	template<typename Float>
	Float softclipAtan2(Float sample, Float ceiling, Float k) noexcept
	{
		const auto a = std::atan(k * sample);
		const auto b = std::atan(k);

		return ceiling * a / b;
	}

	template<typename Float>
	Float softclipFuzzExponential(Float sample, Float ceiling, Float k) noexcept
	{
		static constexpr auto zero = static_cast<Float>(0);
		static constexpr auto one = static_cast<Float>(1);
		static constexpr auto e = static_cast<Float>(2.71828182845904523536028747135266249775724709369995);
		
		const auto sign = sample < zero ? -one : one;
		
		return ceiling * sign * (one - std::pow(e, -std::abs(k * sample))) / (one - std::pow(e, -k));
	}

	template<typename Float>
	Float softclipFiresledge(Float sample, Float ceiling, Float k) noexcept
	{
		static constexpr auto zero = static_cast<Float>(0);
		static constexpr auto one = static_cast<Float>(1);

		const auto sign = sample < zero ? -one : one;
		
		const auto a = std::pow(std::abs(sample), k);
		const auto b = std::tanh(a);
		const auto c = std::pow(b, one / k);

		return ceiling * sign * c;
	}

	template<typename Float>
	Float softclipPrismaHeavy(Float sample, Float ceiling, Float k) noexcept
	{
		static constexpr auto zero = static_cast<Float>(0);
		static constexpr auto one = static_cast<Float>(1);
		
		const auto val = one - k * static_cast<Float>(.99);
		const auto sign = sample < zero ? -one : one;
		
		const auto func = std::pow(std::abs(sample), val) * sign;
		const auto gain = one - (one - val * val * static_cast<Float>(.92));

		return ceiling * func * gain;
	}

	template float hardclip(float, float) noexcept;
	template double hardclip(double, double) noexcept;

	template float ratioclip(float, float, float) noexcept;
	template double ratioclip(double, double, double) noexcept;

	template float softclipCubic(float, float) noexcept;
	template double softclipCubic(double, double) noexcept;

	template float softclipAtan(float, float, float) noexcept;
	template double softclipAtan(double, double, double) noexcept;

	template float softclipSigmoid(float, float, float) noexcept;
	template double softclipSigmoid(double, double, double) noexcept;

	template float softclipAtan2(float, float, float) noexcept;
	template double softclipAtan2(double, double, double) noexcept;

	template float softclipFuzzExponential(float, float, float) noexcept;
	template double softclipFuzzExponential(double, double, double) noexcept;

	template float softclipFiresledge(float, float, float) noexcept;
	template double softclipFiresledge(double, double, double) noexcept;

	template float softclipPrismaHeavy(float, float, float) noexcept;
	template double softclipPrismaHeavy(double, double, double) noexcept;
}