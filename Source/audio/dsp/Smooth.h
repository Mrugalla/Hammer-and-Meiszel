#pragma once
#include "../Using.h"

namespace dsp
{
	namespace smooth
	{
		// a block-based parameter smoother.
		template<typename Float>
		struct Block
		{
			// startVal
			Block(Float = static_cast<Float>(0));

			// not sure if this method makes sense lol
			// bufferOut, bufferIn, numSamples
			void operator()(Float*, Float*, int) noexcept;

			// buffer, val, numSamples
			void operator()(Float*, Float, int) noexcept;

			// buffer, numSamples
			void operator()(Float*, int) noexcept;

			Float curVal;
		};

		using BlockF = Block<float>;
		using BlockD = Block<double>;

		// Float-Type, AutoGain 
		template<typename Float, bool AutoGain>
		struct Lowpass
		{
			// decay
			static Float getXFromFc(Float) noexcept;
			// decay, Fs
			static Float getXFromHz(Float, Float) noexcept;

			// decay
			void makeFromDecayInSamples(Float) noexcept;
			// decay, Fs
			void makeFromDecayInSecs(Float, Float) noexcept;
			// fc
			void makeFromDecayInFc(Float) noexcept;
			// decay, Fs
			void makeFromDecayInHz(Float, Float) noexcept;
			// decay, Fs
			void makeFromDecayInMs(Float, Float) noexcept;

			void copyCutoffFrom(const Lowpass<Float, AutoGain>&) noexcept;

			// startVal, autogain
			Lowpass(const Float = static_cast<Float>(0));

			// resets to startVal
			void reset();

			// value
			void reset(Float);

			// buffer, val, numSamples
			void operator()(Float*, Float, int) noexcept;
			// buffer, numSamples
			void operator()(Float*, int) noexcept;
			// val
			Float operator()(Float) noexcept;

			void setX(Float) noexcept;

			Float a0, b1, y1, startVal;

			Float processSample(Float) noexcept;
		};

		using LowpassF = Lowpass<float, false>;
		using LowpassD = Lowpass<double, false>;
		using LowpassFGain = Lowpass<float, true>;
		using LowpassDGain = Lowpass<double, true>;

		template<typename Float>
		struct Smooth
		{
			// smoothLenMs, Fs
			void makeFromDecayInMs(Float, Float) noexcept;

			// freqHz, Fs
			void makeFromFreqInHz(Float, Float) noexcept;

			// startVal
			Smooth(Float = static_cast<Float>(0));

			void operator=(Smooth<Float>& other) noexcept
			{
				block.curVal = other.block.curVal;
				lowpass.copyCutoffFrom(other.lowpass);
				cur = other.cur;
				dest = other.dest;
				smoothing = other.smoothing;
			}

			// bufferOut, bufferIn, numSamples
			void operator()(Float*, Float*, int) noexcept;

			// buffer, val, numSamples
			bool operator()(Float*, Float, int) noexcept;

			// buffer, val, startIdx, endIdx
			bool operator()(Float*, Float, int, int) noexcept;

			// buffer, numSamples
			bool operator()(Float*, int) noexcept;

			// value (this method is not for parameters!)
			Float operator()(Float) noexcept;

		protected:
			Block<Float> block;
			Lowpass<Float, false> lowpass;
			Float cur, dest;
			bool smoothing;
		};

		using SmoothF = Smooth<float>;
		using SmoothD = Smooth<double>;
	}
}