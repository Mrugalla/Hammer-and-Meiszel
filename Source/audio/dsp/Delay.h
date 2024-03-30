#pragma once
#include "../../libs/MoogLadders-master/src/KrajeskiModel.h"
#include "../../arch/XenManager.h"
#include "PRM.h"
#include "WHead.h"

namespace dsp
{
	class CombFilter
	{
		static constexpr double PB = 0x3fff;
		static constexpr double PBInv = 1. / PB;

		struct Val
		{
			Val();

			double getDelaySamples(const arch::XenManager& xen, double Fs) noexcept;

			double freqHz, pitchNote, pitchParam, pb;
		};

		struct DelayFeedback
		{
			using Lowpass = moog::KrajeskiMoog;

			DelayFeedback();

			// sampleRate, size
			void prepare(double, int);

			// dampFreqHz, numChannels
			void setDampFreqHz(double, int) noexcept;

			// samples, wHead, rHead, feedback[-1,1], numChannels, startIdx, endIdx
			void operator()(double**, const int*, const double*, double, int, int, int) noexcept;

		protected:
			AudioBuffer ringBuffer;
			std::array<Lowpass, 2> lowpass;
			int size;
		};

		static constexpr double LowestFrequencyHz = 20.;

	public:
		CombFilter(const arch::XenManager&);

		// sampleRate
		void prepare(double);

		// samples, midi, wHead, feedback [-1,1], retune [-n,n]semi, numChannels, numSamples
		void operator()(double**, const MidiBuffer&, const int* wHead, double, double, int, int) noexcept;

	protected:
		const arch::XenManager& xenManager;
		PRMD smooth;
		std::array<double, BlockSize2x> readHead;
		DelayFeedback delay;
		Val val;
		double Fs, sizeF, curDelay;
	public:
		int size;
	private:
		// samples, midi, wHead, feedback, numChannels, s
		void processMIDI(double**, const MidiBuffer&, const int*, double, int, int&) noexcept;

		// samples, wHead, feedback, numChannels, startIdx, endIdx
		void processDelay(double**, const int*, double, int, int, int) noexcept;

		// numChannels
		void updatePitch(int) noexcept;
	};
}