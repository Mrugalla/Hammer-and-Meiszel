#pragma once
#include "../../../../arch/XenManager.h"
#include "../../PRM.h"
#include "../../WHead.h"
#include "../../SleepyDetector.h"

namespace dsp
{
	namespace hnm
	{
		static constexpr double LowestFrequencyHz = 20.;
		//
		static constexpr double PB = 0x3fff;
		static constexpr double PBInv = 1. / PB;
		using XenManager = arch::XenManager;
		
		struct Params
		{
			// retune[-n,n]semi, retuneWidth[-1,1], fb[-1,1], fbEnv[-2,2], fbWidth[-2,2],
			double retune, retuneWidth, fb, fbEnv, fbWidth;
		};

		struct Val
		{
			Val();

			void reset() noexcept;

			// xen, Fs
			void updateDelaySamples(const XenManager&, double) noexcept;

			double freqHz, pitchNote, pitchParam, pb, delaySamples;
		};

		struct DelayFeedback
		{
			DelayFeedback();

			// delaySize
			void prepare(int _size);

			// samples, wHead, rHead, feedbackBuffer, numSamples, ch
			void operator()(double**, const int*, const double*,
				const double*, int, int) noexcept;
		private:
			AudioBuffer ringBuffer;
			int size;
		};

		struct ReadHead
		{
			ReadHead();

			// size
			void prepare(double);

			// delayBuf, wHead, numSamples
			void operator()(double*, const int*, int) noexcept;
		private:
			double size;
		};

		struct Voice
		{
			Voice();

			// sampleRate
			void prepare(double);

			// samples, xenManager, params, envGenMod[-1,1], numChannels, numSamples
			void operator()(double**, const XenManager&,
				const Params&, double, int, int) noexcept;

			void triggerNoteOn(double) noexcept;

			void triggerNoteOff() noexcept;

			void triggerPitchbend(double) noexcept;

			bool isRinging() const noexcept;

		private:
			WHead1x wHead;
			std::array<PRMD, 2> delayPRMs, feedbackPRMs;
			ReadHead readHead;
			DelayFeedback delay;
			std::array<Val, 2> vals;
			double Fs;
			arch::XenManager::Info xenInfo;
			SleepyDetector sleepy;
		public:
			int size;
		private:
			// xenManager
			bool xenUpdated(const XenManager&) noexcept;

			// xenManager, params, envGenMod, numChannels, numSamples
			void updateParams(const XenManager&, const Params&, double, int, int) noexcept;

			// xenManager, numChannels
			void updatePitch(const XenManager&, int) noexcept;

			// samples, numChannels, numSamples
			void applyDelay(double**, int, int) noexcept;
		};

		using VoicesArray = std::array<Voice, NumMPEChannels>;

		struct Comb
		{
			Comb();

			// sampleRate
			void prepare(double);

			// samples, xenManager, params, envGenMod, numChannels, numSamples, v
			void operator()(double**, const arch::XenManager&,
				const Params&, double, int, int, int) noexcept;

			// noteNumber, v
			void triggerNoteOn(double, int) noexcept;

			// v
			void triggerNoteOff(int) noexcept;

			// pitchbend, v
			void triggerPitchbend(double, int) noexcept;

			bool isRinging(int) const noexcept;

		protected:
			VoicesArray voices;
		};
	}
}

// todo
// 
//params
	//chord type[+5, +7, +12, +17, +19, +24]
	//chord depth[0, 1]
	//dry blend[-inf, 0]
	//wet gain[-inf, 0]
	//flanger/phaser switch