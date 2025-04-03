#pragma once
#include "../Using.h"

namespace dsp
{
	struct Transport
	{
		struct PhaseInfo
		{
			double phase, inc;
		};

		struct Info
		{
			Info() :
				ppq(0.),
				bpm(120.),
				timeSecs(0.),
				numerator(3.),
				denominator(4.),
				timeSamples(0),
				playing(true)
			{
			}

			double getBeatsPerSec() const noexcept
			{
				return bpm * SixtyInv;
			}

			PhaseInfo getPhaseInfo(double rateSync, double sampleRateInv) const noexcept
			{
				const auto d4 = denominator / 4.;
				const auto r = d4 / (rateSync * numerator);

				const auto beatsPerSecs = getBeatsPerSec();
				const auto inc = beatsPerSecs * sampleRateInv * r;

				const auto pos = ppq * r;

				return { pos, inc };
			}

			double ppq, bpm, timeSecs, numerator, denominator;
			Int64 timeSamples;
			bool playing;
		};

		Transport() :
			info(),
			sampleRateInv(0.)
		{
		}

		void prepare(double _sampleRateInv) noexcept
		{
			sampleRateInv = _sampleRateInv;
		}

		void operator()(const PlayHead* playHead) noexcept
		{
			if (juce::JUCEApplicationBase::isStandaloneApp() || playHead == nullptr)
				return;
			const auto phx = playHead->getPosition();
			if (!phx.hasValue())
				return;
			const auto& posInfo = *phx;
			update(posInfo);
		}

		void operator()(int numSamples) noexcept
		{
			proceed(numSamples);
		}

		Info info;
	private:
		double sampleRateInv;

		void update(const PlayHead::PositionInfo& phx) noexcept
		{
			const auto ppq = phx.getPpqPosition();
			const auto bpm = phx.getBpm();
			const auto timeSecs = phx.getTimeInSeconds();
			const auto timeSig = phx.getTimeSignature();
			const auto timeSamples = phx.getTimeInSamples();
			
			info.ppq = ppq.hasValue() ? *ppq : 0.;
			info.bpm = bpm.hasValue() ? *bpm : 90.;
			info.timeSecs = timeSecs.hasValue() ? *timeSecs : 0.;
			if (timeSig.hasValue())
			{
				const auto& ts = *timeSig;
				info.denominator = ts.denominator;
				info.numerator = ts.numerator;
			}
			info.timeSamples = timeSamples.hasValue() ? *timeSamples : 0;
			info.playing = phx.getIsPlaying();
		}

		void proceed(int numSamples) noexcept
		{
			info.timeSamples += numSamples;
			info.timeSecs = static_cast<double>(info.timeSamples) * sampleRateInv;
			info.ppq += static_cast<double>(numSamples) * sampleRateInv * info.bpm * SixtyInv;
		}
	};
}