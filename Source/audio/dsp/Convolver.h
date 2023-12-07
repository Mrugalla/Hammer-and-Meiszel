#pragma once
#include "../Using.h"

namespace dsp
{
	// size 1 << 8
	template<typename Float, int Size>
	struct ImpulseResponse
	{
		using Buffer = std::array<Float, Size>;

		ImpulseResponse();

		Float& operator[](int);

		const Float& operator[](int) const;

		/*
		* Fs, fc, bw, normalize
		nyquist == Fs / 2
		fc < nyquist
		bw < nyquist
		fc + bw < nyquist
		*/
		void makeLowpass(Float, Float, Float, bool);

		/*
		* Fs, fc, normalize
		nyquist == Fs / 2
		fc < nyquist
		*/
		void makeLowpass(Float, Float, bool);

		/*
		* Fs, fc, bw
		nyquist == Fs / 2
		fc < nyquist
		bw < nyquist
		*/
		void makeHighpass(Float, Float, Float);

		int getLatency() const noexcept;

	private:
		Buffer buffer;
	public:
		int size;
	};
	
	template<typename Float, int Size>
	struct Convolver
	{
		using Buffer = std::array<std::array<Float, Size>, NumChannels>;

		Convolver(const ImpulseResponse<Float, Size>&);

		/* samples, wHead, numChannels, numSamples */
		void processBlock(Float* const*, const int*, int, int) noexcept;

		/* smpls, ring, whead, numSamples */
		void processBlock(Float*, Float*, const int*, int) noexcept;

		/* smpl, ring, w */
		Float processSample(Float, Float*, int) noexcept;

	private:
		const ImpulseResponse<Float, Size>& ir;
	public:
		Buffer ringBuffer;
	};

	template struct ImpulseResponse<float, 1 << 8>;
	template struct ImpulseResponse<double, 1 << 8>;
	template struct Convolver<float, 1 << 8>;
	template struct Convolver<double, 1 << 8>;

	//template struct ImpulseResponse<float, 1 << 15>;
	//template struct ImpulseResponse<double, 1 << 15>;
	//template struct Convolver<float, 1 << 15>;
	//template struct Convolver<double, 1 << 15>;

}