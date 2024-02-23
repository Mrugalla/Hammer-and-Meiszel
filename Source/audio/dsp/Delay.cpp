#include "Delay.h"
#include "../../arch/Math.h"

namespace dsp
{
	CombFilter::Val::Val() :
		pitch(0.),
		pb(0.)
	{}

	double CombFilter::Val::getDelaySamples(const arch::XenManager& xen, double Fs) noexcept
	{
		const auto freqHz = xen.noteToFreqHzWithWrap(pitch + pb, LowestFrequencyHz);
		return math::freqHzToSamples(freqHz, Fs);
	}

	CombFilter::DelayFeedback::DelayFeedback() :
		ringBuffer(),
		//lowpass{ 0.f, 0.f },
		size(0)
	{}

	void CombFilter::DelayFeedback::prepare(int _size)
	{
		size = _size;
		ringBuffer.setSize(2, size, false, true, false);
		//for (auto& lp : lowpass)
		//	lp.makeFromDecayInHz(1000.f, Fs);
	}

	void CombFilter::DelayFeedback::operator()(double** samples,
		const int* wHead, const double* rHead, double fb,
		int numChannels, int startIdx, int endIdx) noexcept
	{
		auto ringBuf = ringBuffer.getArrayOfWritePointers();

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto ring = ringBuf[ch];
			//auto& lp = lowpass[ch];

			for (auto s = startIdx; s < endIdx; ++s)
			{
				//lp.setX(dampBuf[s]);

				const auto w = wHead[s];
				const auto r = rHead[s];
				//const auto fb = fbBuf[s];

				//const auto sOut = lp(interpolate::cubicHermiteSpline(ring, r, size)) * fb + smpls[s];
				const auto sOut = math::cubicHermiteSpline(ring, r, size) * fb + smpls[s];
				const auto sIn = sOut;

				ring[w] = sIn;
				smpls[s] = sOut;
			}
		}
	}

	// CombFilter

	CombFilter::CombFilter(const arch::XenManager& _xenManager) :
		xenManager(_xenManager),
		smooth(0.),
		readHead(),
		delay(),
		val(),
		Fs(0.), sizeF(0.), curDelay(0.),
		size(0)
	{}

	void CombFilter::prepare(double sampleRate)
	{
		Fs = sampleRate;
		sizeF = std::ceil(math::freqHzToSamples(LowestFrequencyHz, Fs));
		size = static_cast<int>(sizeF);
		delay.prepare(size);
		smooth.prepare(Fs, 1.);
		curDelay = 0.;
	}

	void CombFilter::operator()(double** samples, const MidiBuffer& midi, const int* wHead,
		double feedback, double retune, int numChannels, int numSamples) noexcept
	{
		static constexpr double PB = 0x3fff;
		static constexpr double PBInv = 1. / PB;

		auto s = 0;
		for (const auto it : midi)
		{
			const auto msg = it.getMessage();
			if (msg.isNoteOn())
			{
				const auto ts = it.samplePosition;
				const auto note = msg.getNoteNumber();
				val.pitch = static_cast<double>(note) + retune;
				curDelay = val.getDelaySamples(xenManager, Fs);
				processDelay(samples, wHead, feedback, numChannels, s, ts);
				s = ts;
			}
			else if (msg.isPitchWheel())
			{
				const auto ts = it.samplePosition;
				val.pb = (static_cast<double>(msg.getPitchWheelValue()) * PBInv - .5) * xenManager.getPitchbendRange() * 2.;
				curDelay = val.getDelaySamples(xenManager, Fs);
				processDelay(samples, wHead, feedback, numChannels, s, ts);
				s = ts;
			}
		}
		processDelay(samples, wHead, feedback, numChannels, s, numSamples);
	}

	void CombFilter::processDelay(double** samples, const int* wHead, double feedback, int numChannels, int startIdx, int endIdx) noexcept
	{
		const auto smoothInfo = smooth(curDelay, endIdx - startIdx);
		if(smoothInfo.smoothing)
			for (auto s = startIdx; s < endIdx; ++s)
			{
				const auto w = static_cast<float>(wHead[s]);
				auto r = w - smoothInfo[s - startIdx];
				if (r < 0.f)
					r += sizeF;
				readHead[s] = r;
			}
		else
			for (auto s = startIdx; s < endIdx; ++s)
			{
				const auto w = static_cast<float>(wHead[s]);
				auto r = w - smoothInfo.val;
				if (r < 0.f)
					r += sizeF;
				readHead[s] = r;
			}

		delay
		(
			samples,
			wHead, readHead.data(),
			feedback,
			numChannels, startIdx, endIdx
		);
	}
}