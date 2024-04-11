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

	template<typename Float>
	struct PRMBlock
	{
		/* startVal */
		PRMBlock(Float _startVal = static_cast<Float>(0)) :
			startVal(_startVal),
			lp(startVal),
			info(nullptr, startVal, false)
		{}

		/* sampleRate, smoothLenMs */
		void prepare(Float sampleRate, Float smoothLenMs) noexcept
		{
			const auto blockSize = static_cast<double>(BlockSize);
			const auto smoothLenBlock = smoothLenMs / blockSize;
			lp.makeFromDecayInMs(smoothLenBlock, sampleRate);
		}

		/* value */
		PRMInfo<Float> operator()(Float x) noexcept
		{
			if (info.val != x)
			{
				info.smoothing = true;
				info.val = lp(x);
				if (info.val == x)
					info.smoothing = false;
			}
			return info;
		}

		operator Float() const noexcept
		{
			return info.val;
		}

	protected:
		double startVal;
		smooth::Lowpass<Float, false> lp;
		PRMInfo<Float> info;
	};

	using PRMBlockF = PRMBlock<float>;
	using PRMBlockD = PRMBlock<double>;
}