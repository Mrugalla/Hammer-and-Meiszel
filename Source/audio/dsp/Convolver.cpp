#include "Convolver.h"
#include <cmath>

namespace dsp
{
	template<typename Float, int Size>
	ImpulseResponse<Float, Size>::ImpulseResponse() :
		buffer(),
		size(0)
	{
	}

	template<typename Float, int Size>
	Float& ImpulseResponse<Float, Size>::operator[](int i)
	{
		return buffer[i];
	}

	template<typename Float, int Size>
	const Float& ImpulseResponse<Float, Size>::operator[](int i) const
	{
		return buffer[i];
	}

	template<typename Float, int Size>
	void ImpulseResponse<Float, Size>::makeLowpass(Float Fs, Float fc,
		Float bw, bool normalize)
	{
		const auto nyquist = Fs * static_cast<Float>(.5);
		if (fc > nyquist || bw > nyquist || fc + bw > nyquist)
			return;

		fc /= Fs;
		bw /= Fs;

		auto M = static_cast<int>(static_cast<Float>(4.) / bw);
		if (M % 2 != 0)
			++M; // M is even number

		const auto Mf = static_cast<Float>(M);
		const auto MHalf = Mf * static_cast<Float>(.5);
		const auto MInv = static_cast<Float>(1.) / Mf;

		size = std::min(Size, M + 1);

		const auto h = [&](Float i)
		{ // sinc
			i -= MHalf;
			if (i != 0.)
				return std::sin(static_cast<Float>(Tau) * fc * i) / i;
			return static_cast<Float>(Tau) * fc;
		};

		const auto w = [&, tau2 = static_cast<Float>(Tau) * static_cast<Float>(2.)](Float i)
		{ // blackman window
			i *= MInv;
			return static_cast<Float>(.42) - static_cast<Float>(.5) * std::cos(static_cast<Float>(Tau) * i) + static_cast<Float>(.08) * std::cos(tau2 * i);
		};

		for (auto n = 0; n < size; ++n)
		{
			auto nF = static_cast<Float>(n);
			buffer[n] = h(nF) * w(nF);
		}

		if (normalize)
		{
			auto sum = static_cast<Float>(0);
			for (auto n = 0; n < size; ++n)
				sum += buffer[n];
			const auto sumInv = static_cast<Float>(1) / sum;
			for (auto n = 0; n < size; ++n)
				buffer[n] *= sumInv;
		}
	}

	template<typename Float, int Size>
	void ImpulseResponse<Float, Size>::makeLowpass(Float Fs, Float fc, bool normalize)
	{
		const auto bw = Fs * static_cast<Float>(.25) - fc - static_cast<Float>(1);
		makeLowpass(Fs, fc, bw, normalize);
	}

	template<typename Float, int Size>
	void ImpulseResponse<Float, Size>::makeHighpass(Float Fs, Float fc, Float bw)
	{
		makeLowpass(Fs, fc, bw, false);
		for(auto i = 0; i < size; ++i)
			buffer[i] *= static_cast<Float>(-1);
		buffer[size / 2] += static_cast<Float>(1);
	}

	template<typename Float, int Size>
	int ImpulseResponse<Float, Size>::getLatency() const noexcept
	{
		return size / 2;
	}

	template<typename Float, int Size>
	Convolver<Float, Size>::Convolver(const ImpulseResponse<Float, Size>& ir) :
		ir(ir),
		ringBuffer()
	{
	}

	template<typename Float, int Size>
	void Convolver<Float, Size>::processBlock(Float* const* samples, const int* wHead,
		int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto ring = ringBuffer[ch].data();

			processBlock(smpls, ring, wHead, numSamples);
		}
	}

	template<typename Float, int Size>
	void Convolver<Float, Size>::processBlock(Float* smpls, Float* ring,
		const int* wHead, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
			smpls[s] = processSample(smpls[s], ring, wHead[s]);
	}

	template<typename Float, int Size>
	Float Convolver<Float, Size>::processSample(Float smpl, Float* ring, int w) noexcept
	{
		ring[w] = smpl;

		auto r = w;
		auto y = ring[r] * ir[0];

		const auto size = ir.size;
		const auto max = size - 1;
		for (auto i = 1; i < size; ++i)
		{
			--r;
			if (r == -1)
				r = max;

			y += ring[r] * ir[i];
		}

		return y;
	}

	template struct ImpulseResponse<double, 1 << 8>;
	template struct Convolver<double, 1 << 8>;

	template struct ImpulseResponse<float, 1 << 15>;
	template struct Convolver<float, 1 << 15>;
}