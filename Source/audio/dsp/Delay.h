#pragma once
#include "../../libs/MoogLadders-master/src/Filters.h"
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

		template<size_t NumSequences, size_t SeriesSize>
		struct AllpassSeries
		{
			static constexpr int Slope = 4;
			using AllpassSlope = moog::BiquadFilterSlope<Slope>;
			using AllpassSlopeChannel = std::array<AllpassSlope, 2>;

			AllpassSeries() :
				filters(),
				sequences(),
				sampleRateInv(1.)
			{
				for (auto& apSlopeChannels : filters)
				{
					for (auto& apSlope : apSlopeChannels)
					{
						apSlope.setType(moog::BiquadFilter::Type::AP);
						apSlope.setSlope(Slope);
					}
				}
			}

			void prepare(double _sampleRateInv)
			{
				sampleRateInv = _sampleRateInv;
				for (auto& apSlopeChannels : filters)
				{
					for (auto& apSlope : apSlopeChannels)
					{
						apSlope.prepare();
						apSlope.setCutoffFc(1000. * sampleRateInv);
						apSlope.setResonance(2.);
						apSlope.update();
					}
				}
			}

			void setDampFreqHz(double dampFreqHz, double sequencesPosition, int numChannels) noexcept
			{
				const auto sequencesPosAbs = sequencesPosition * static_cast<double>(NumSequences);
				const auto sequenceLowIdx = static_cast<int>(sequencesPosAbs);
				const auto sequenceHighIdx = sequenceLowIdx + 1;
				const auto sequencesPosFrac = sequencesPosAbs - static_cast<double>(sequenceLowIdx);
				auto& sequenceLow = sequences[sequenceLowIdx];
				auto& sequenceHigh = sequences[sequenceHighIdx];
				double sequence[SeriesSize];
				for (auto i = 0; i < SeriesSize; ++i)
					sequence[i] = sequenceLow[i] + sequencesPosFrac * (sequenceHigh[i] - sequenceLow[i]);

				for (auto i = 0; i < SeriesSize; ++i)
				{
					const auto sequenceVal = sequence[i];
					auto& apSlopeChannels = filters[i];
					//const auto iF = static_cast<double>(i);
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto& apSlope = apSlopeChannels[ch];
						auto fc = sequenceVal * dampFreqHz * sampleRateInv;
						if (fc > 0.499999)
							fc = 0.499999;
						apSlope.setCutoffFc(fc);
						apSlope.update();
						apSlope.reset();
					}
				}
			}

			void setAPReso(double apReso, int numChannels) noexcept
			{
				for (auto& apSlopeChannels : filters)
				{
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto& apSlope = apSlopeChannels[ch];
						apSlope.setResonance(apReso);
						apSlope.update();
						apSlope.reset();
					}
				}
			}

			double operator()(double smpl, int ch) noexcept
			{
				auto y = smpl;
				for (auto i = 0; i < SeriesSize; ++i)
				{
					auto& apSlopeChannels = filters[i];
					auto& apSlope = apSlopeChannels[ch];
					y = apSlope(y);
				}
				return y;
			}

			void addSequence(std::array<double, SeriesSize>&& sequence, int idx) noexcept
			{
				sequences[idx] = sequence;
				if (idx == NumSequences - 1)
				{
					sequences[NumSequences] = sequence;
					sequences[NumSequences + 1] = sequence;
				}
			}

			std::array<AllpassSlopeChannel, SeriesSize> filters;
			std::array<std::array<double, SeriesSize>, NumSequences + 2> sequences;
			double sampleRateInv;
		};

		struct DelayFeedback
		{
			DelayFeedback();

			// sampleRate, size
			void prepare(double, int);

			// dampFreqHz, sequencesPos, numChannels
			void setDampFreqHz(double, double, int) noexcept;

			// apReso, numChannels
			void setAPReso(double, int) noexcept;

			// samples, wHead, rHead, feedback[-1,1], apResoInfo, numChannels, startIdx, endIdx
			void operator()(double**, const int*, const double*, double, PRMInfoD, int, int, int) noexcept;

		protected:
			AudioBuffer ringBuffer;
			AllpassSeries<3, 6> allpassSeries;
			double sampleRateInv;
			int size;
		};

		static constexpr double LowestFrequencyHz = 20.;

	public:
		CombFilter(const arch::XenManager&);

		// sampleRate
		void prepare(double);

		void updateParameters(double _sequencesPos, int numChannels) noexcept
		{
			sequencesPos = _sequencesPos;
			updatePitch(numChannels);
		}

		// samples, midi, wHead, feedback [-1,1], retune [-n,n]semi, apReso[.1, 8], numChannels, numSamples
		void operator()(double**, const MidiBuffer&, const int*, double, double, double, int, int) noexcept;

	protected:
		const arch::XenManager& xenManager;
		PRMD smooth;
		std::array<double, BlockSize2x> readHead;
		PRMD apResoPRM;
		DelayFeedback delay;
		Val val;
		double Fs, sizeF, curDelay, sequencesPos;
	public:
		int size;
	private:
		// samples, midi, wHead, feedback, numChannels, s
		void processMIDI(double**, const MidiBuffer&, const int*, PRMInfoD, double, int, int&) noexcept;

		// samples, wHead, apResoInfo, feedback, numChannels, startIdx, endIdx
		void processDelay(double**, const int*, PRMInfoD, double, int, int, int) noexcept;

		// numChannels
		void updatePitch(int) noexcept;
	};
}