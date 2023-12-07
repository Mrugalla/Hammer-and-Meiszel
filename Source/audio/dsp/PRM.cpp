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
		if (!smoothing)
			SIMD::fill(buf, val, numSamples);
	}

	template struct PRMInfo<float>;
	template struct PRMInfo<double>;

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
}