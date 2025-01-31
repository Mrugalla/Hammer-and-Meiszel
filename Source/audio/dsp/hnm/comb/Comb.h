#pragma once
#include "../../../../arch/XenManager.h"
#include "../../PRM.h"
#include "../../WHead.h"

namespace dsp
{
	namespace hnm
	{
		static constexpr double LowestFrequencyHz = 20.;
		//
		static constexpr double PB = 0x3fff;
		static constexpr double PBInv = 1. / PB;
		using XenManager = arch::XenManager;
		
		struct Val
		{
			Val();

			void reset() noexcept;

			// xen, Fs
			double getDelaySamples(const XenManager&, double) noexcept;

			double freqHz, pitchNote, pitchParam, pb, delaySamples;
		};

		struct DelayFeedback
		{
			DelayFeedback();

			// sampleRate, delaySize
			void prepare(double sampleRate, int _size);

			// samples, wHead, rHead, feedbackBuffer, numSamples, ch
			void operator()(double**, const int*, const double*,
				const double*, int, int) noexcept;

			bool isRinging() const noexcept;

		private:
			AudioBuffer ringBuffer;
			int size;

			int sleepyTimer, sleepyThreshold;
			bool ringing;
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

			// samples, midi, wHead, xen,
			// retune[-n,n]semi, retuneWidth[-1,1],
			// feedback[-1,1], feedbackEnv[-2,2], feedbackWidth[-2,2],
			// envGenMod[-1,1], numChannels, numSamples
			void operator()(double**, const MidiBuffer&,
				const int*, const XenManager&,
				double, double,
				double, double, double, double,
				int, int) noexcept;

			bool isRinging() const noexcept;

		private:
			std::array<PRMD, 2> delayPRMs, feedbackPRMs;
			std::array<PRMInfoD, 2> feedbackInfos;
			ReadHead readHead;
			DelayFeedback delay;
			std::array<Val, 2> vals;
			double Fs;
			double xen, anchor, masterTune;
		public:
			int size;
		private:
			// xen, retune, retuneWidth, numChannels
			void updateRetuneParams(const XenManager&,
				double, double, int) noexcept;

			// fb, fbWidth, fbEnv, envGenMod, numChannels, numSamples
			void updateFeedbackParams(double, double,
				double, double, int, int) noexcept;

			// midi, xenManager, numChannels, numSamples
			void processMIDI(const MidiBuffer&,
				const XenManager&, int, int) noexcept;

			// xenManager, numChannels, startIdx, endIdx
			void updatePitch(const XenManager&, int, int, int) noexcept;

			// xenManager, ch
			void updatePitch(const XenManager&, int) noexcept;
		};

		using VoicesArray = std::array<Voice, NumMPEChannels>;

		struct Comb
		{
			Comb();

			// sampleRate
			void prepare(double);

			// numSamples
			void operator()(int) noexcept;

			// samples, midi, xen, retune, retuneWidth, feedback, feedbackEnv, feedbackWidth,
			// envGenMod, numChannels, numSamples, voiceIdx
			void operator()(double**, const MidiBuffer&,
				const arch::XenManager&, double, double,
				double, double, double, double, int, int, int) noexcept;

			bool isRinging(int) const noexcept;

		protected:
			WHead1x wHead;
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