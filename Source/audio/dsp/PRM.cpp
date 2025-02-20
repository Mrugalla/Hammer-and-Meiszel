#include "PRM.h"

namespace dsp
{
	template<typename Float>
	PRMInfo<Float>::PRMInfo(Float* _buf, Float _val, bool _smoothing) :
		buf(_buf),
		val(_val),
		smoothing(_smoothing)
	{}

	template<typename Float>
	Float PRMInfo<Float>::operator[](int i) const noexcept
	{
		return buf[i];
	}

	template<typename Float>
	void PRMInfo<Float>::copyToBuffer(int numSamples) noexcept
	{
		if (smoothing)
			return;
		SIMD::fill(buf, val, numSamples);
	}

	template<typename Float>
	void PRMInfo<Float>::copyToBuffer(int startIdx, int endIdx) noexcept
	{
		if (smoothing)
			return;
		const auto numSamples = endIdx - startIdx;
		SIMD::fill(&buf[startIdx], val, numSamples);
	}

	template<typename Float>
	PRMInfo<Float>::operator Float() const noexcept
	{
		return val;
	}

	template struct PRMInfo<float>;
	template struct PRMInfo<double>;

	//

	template<typename Float>
	PRM<Float>::PRM(Float startVal) :
		buf(),
		smooth(startVal),
		value(startVal)
	{
		for(auto& b : buf)
			b = startVal;
	}
	
	template<typename Float>
	void PRM<Float>::prepare(Float sampleRate, Float smoothLenMs) noexcept
	{
		smooth.makeFromDecayInMs(smoothLenMs, sampleRate);
	}

	template<typename Float>
	PRMInfo<Float> PRM<Float>::operator()(Float val, int numSamples) noexcept
	{
		value = val;
		bool smoothing = smooth(buf.data(), value, numSamples);
		return { buf.data(), value, smoothing };
	}

	template<typename Float>
	PRMInfo<Float> PRM<Float>::operator()(Float val, int startIdx, int endIdx) noexcept
	{
		value = val;
		return operator()(startIdx, endIdx);
	}

	template<typename Float>
	PRMInfo<Float> PRM<Float>::operator()(int startIdx, int endIdx) noexcept
	{
		bool smoothing = smooth(buf.data(), value, startIdx, endIdx);
		return { buf.data(), value, smoothing };
	}


	template<typename Float>
	PRMInfo<Float> PRM<Float>::operator()(int numSamples) noexcept
	{
		bool smoothing = smooth(buf.data(), value, numSamples);
		return { buf.data(), value, smoothing };
	}

	template<typename Float>
	Float PRM<Float>::operator[](int i) const noexcept
	{
		return buf[i];
	}

	template struct PRM<float>;
	template struct PRM<double>;

	//

	template<typename Float>
	PRMBlock<Float>::PRMBlock(Float _startVal) :
		startVal(_startVal),
		lp(startVal),
		info(nullptr, startVal, false)
	{}

	template<typename Float>
	void PRMBlock<Float>::prepare(Float sampleRate, Float smoothLenMs) noexcept
	{
		const auto blockSize = static_cast<Float>(BlockSize);
		const auto smoothLenBlock = smoothLenMs / blockSize;
		lp.makeFromDecayInMs(smoothLenBlock, sampleRate);
	}

	template<typename Float>
	PRMInfo<Float> PRMBlock<Float>::operator()(Float x) noexcept
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

	template<typename Float>
	PRMBlock<Float>::operator Float() const noexcept
	{
		return info;
	}

	template struct PRMBlock<float>;
	template struct PRMBlock<double>;

	//

	template<typename Float>
	PRMBlockStereo<Float>::PRMBlockStereo(Float startVal) :
		prms{ startVal, startVal }
	{}

	template<typename Float>
	void PRMBlockStereo<Float>::prepare(Float sampleRate, Float smoothLenMs) noexcept
	{
		for (auto& prm : prms)
			prm.prepare(sampleRate, smoothLenMs);
	}

	template<typename Float>
	PRMInfo<Float> PRMBlockStereo<Float>::operator()(Float x, int ch) noexcept
	{
		return prms[ch](x);
	}

	template<typename Float>
	PRMInfo<Float> PRMBlockStereo<Float>::operator[](int ch) const noexcept
	{
		return prms[ch].info;
	}

	template struct PRMBlockStereo<float>;
	template struct PRMBlockStereo<double>;
}