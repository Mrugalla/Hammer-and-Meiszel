#pragma once
#include <cmath>
#include <stdint.h>

namespace moog
{
	static constexpr double MOOG_E = 2.71828182845904523536028747135266250;
	static constexpr double  MOOG_LOG2E = 1.44269504088896340735992468100189214;
	static constexpr double  MOOG_LOG10E = 0.434294481903251827651128918916605082;
	static constexpr double  MOOG_LN2 = 0.693147180559945309417232121458176568;
	static constexpr double  MOOG_LN10 = 2.30258509299404568401799145468436421;
	static constexpr double  MOOG_PI = 3.14159265358979323846264338327950288;
	static constexpr double  MOOG_TAU = 6.28318530717958647692528676655900576;
	static constexpr double  MOOG_PI_2 = 1.57079632679489661923132169163975144;
	static constexpr double  MOOG_PI_4 = 0.785398163397448309615660845819875721;
	static constexpr double  MOOG_1_PI = 0.318309886183790671537767526745028724;
	static constexpr double  MOOG_2_PI = 0.636619772367581343075535053490057448;
	static constexpr double  MOOG_2_SQRTPI = 1.12837916709551257389615890312154517;
	static constexpr double  MOOG_SQRT2 = 1.41421356237309504880168872420969808;
	static constexpr double  MOOG_SQRT1_2 = 0.707106781186547524400844362104849039;
	static constexpr double  MOOG_INV_PI_2 = 0.159154943091895;

#define NO_COPY(C) C(const C &) = delete; C & operator = (const C &) = delete
#define NO_MOVE(C) NO_COPY(C); C(C &&) = delete; C & operator = (const C &&) = delete

	template<typename T>
	inline T SNAP_TO_ZERO(T n) noexcept
	{
		if (!(n < -1.0e-8 || n > 1.0e-8))
			n = 0;
		return n;
	}

	// Linear interpolation, used to crossfade a gain table
	template<typename T>
	inline T moog_lerp(T amount, T a, T b) noexcept
	{
		return (static_cast<T>(1) - amount) * a + amount * b;
	}

	template<typename T>
	inline T moog_min(T a, T b) noexcept
	{
		a = b - a;
		a += std::abs(a);
		a *= static_cast<T>(.5);
		a = b - a;
		return a;
	}

	// Clamp without branching
	// If input - _limit < 0, then it really substracts, and the 0.5 to make it half the 2 inputs.
	// If > 0 then they just cancel, and keeps input normal.
	// The easiest way to understand it is check what happends on both cases.
	template<typename T>
	inline T moog_saturate(T input) noexcept
	{
		const auto a = static_cast<T>(.95);
		const auto x1 = std::abs(input + a);
		const auto x2 = std::abs(input - a);
		return static_cast<T>(.5) * (x1 - x2);
	}

	// Imitate the (tanh) clipping function of a transistor pair.
	// to 4th order, tanh is x - x*x*x/3; this cubic's
	// plateaus are at +/- 1 so clip to 1 and evaluate the cubic.
	// This is pretty coarse - for instance if you clip a sinusoid this way you
	// can sometimes hear the discontinuity in 4th derivative at the clip point
	template<typename T>
	inline T clip(T value, T saturation, T saturationinverse) noexcept
	{
		const auto one = static_cast<T>(1);
		const auto v2 = (value * saturationinverse > one ? one :
			(value * saturationinverse < -one ? -one :
				value * saturationinverse));
		return (saturation * (v2 - (one / static_cast<T>(3)) * v2 * v2 * v2));
	}

	template<typename T>
	inline T HZ_TO_RAD(T f) noexcept
	{
		return MOOG_PI_2 * f;
	}

	template<typename T>
	inline T RAD_TO_HZ(T omega) noexcept
	{
		return MOOG_INV_PI_2 * omega;
	}

#ifdef __GNUC__
#define ctz(N) __builtin_ctz(N)
#else
	template<typename T>
	inline int ctz(T x)
	{
		int p, b;
		for (p = 0, b = 1; !(b & x); b <<= 1, ++p)
			;
		return p;
	}
#endif

	inline double fast_tanh(double x)
	{
		double x2 = x * x;
		return x * (27. + x2) / (27. + 9. * x2);
	}
}