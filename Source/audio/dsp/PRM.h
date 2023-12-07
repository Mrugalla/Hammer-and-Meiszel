#pragma once
#include "Smooth.h"

namespace dsp
{
	template<typename Float>
	struct PRMInfo
	{
		/* buf, val, smoothing */
		PRMInfo(Float*, Float, bool);

		/* idx */
		Float operator[](int) const noexcept;

		void copyToBuffer(int) noexcept;

		Float* buf;
		Float val;
		bool smoothing;
	};

	using PRMInfoF = PRMInfo<float>;
	using PRMInfoD = PRMInfo<double>;
	
	template<typename Float>
	struct PRM
	{
		/* startVal */
		PRM(Float = static_cast<Float>(0));
		
		/* sampleRate, smoothLenMs */
		void prepare(Float sampleRate, Float smoothLenMs) noexcept;

		/* value, numSamples */
		PRMInfo<Float> operator()(Float, int) noexcept;

		/* value, startIdx, endIdx */
		PRMInfo<Float> operator()(Float, int, int) noexcept;

		/* numSamples */
		PRMInfo<Float> operator()(int) noexcept;

		/* idx */
		Float operator[](int) const noexcept;

		std::array<Float, BlockSize2x> buf;
		smooth::Smooth<Float> smooth;
		Float value;
	};

	using PRMF = PRM<float>;
	using PRMD = PRM<double>;
}