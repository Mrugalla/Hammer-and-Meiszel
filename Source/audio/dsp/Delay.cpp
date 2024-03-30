#include "Delay.h"
#include "../../arch/Math.h"

namespace dsp
{
	CombFilter::Val::Val() :
		freqHz(0.),
		pitchNote(0.),
		pitchParam(0.),
		pb(0.)
	{}

	double CombFilter::Val::getDelaySamples(const arch::XenManager& xen, double Fs) noexcept
	{
		freqHz = xen.noteToFreqHzWithWrap(pitchNote + pitchParam + pb, LowestFrequencyHz);
		return math::freqHzToSamples(freqHz, Fs);
	}

	CombFilter::DelayFeedback::DelayFeedback() :
		ringBuffer(),
		lowpass(),
		size(0)
	{}

	void CombFilter::DelayFeedback::prepare(double sampleRate, int _size)
	{
		size = _size;
		ringBuffer.setSize(2, size, false, true, false);
		for (auto& lp : lowpass)
		{
			lp.prepare(sampleRate);
			lp.setResonance(2000.);
		}
			
	}

	void CombFilter::DelayFeedback::setDampFreqHz(double dampFreqHz, int numChannels) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto& lp = lowpass[ch];
			lp.setCutoff(dampFreqHz);
		}
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
			auto& lp = lowpass[ch];

			for (auto s = startIdx; s < endIdx; ++s)
			{
				const auto w = wHead[s];
				const auto r = rHead[s];

				const auto smplPresent = smpls[s];
				const auto smplDelayed = math::cubicHermiteSpline(ring, r, size);

				const auto sOut = lp(smplDelayed) * fb + smplPresent;
				//const auto sOut = smplDelayed * fb + smplPresent;
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
		delay.prepare(Fs, size);
		smooth.prepare(Fs, 1.);
		curDelay = 0.;
	}

	void CombFilter::operator()(double** samples, const MidiBuffer& midi, const int* wHead,
		double feedback, double retune, int numChannels, int numSamples) noexcept
	{
		if (val.pitchParam != retune)
		{
			val.pitchParam = retune;
			updatePitch(numChannels);
		}

		auto s = 0;
		processMIDI(samples, midi, wHead, feedback, numChannels, s);
		processDelay(samples, wHead, feedback, numChannels, s, numSamples);
	}

	void CombFilter::processMIDI(double** samples, const MidiBuffer& midi, const int* wHead, double feedback, int numChannels, int& s) noexcept
	{
		for (const auto it : midi)
		{
			const auto msg = it.getMessage();
			if (msg.isNoteOn())
			{
				const auto ts = it.samplePosition;
				const auto note = msg.getNoteNumber();
				val.pitchNote = static_cast<double>(note);
				updatePitch(numChannels);
				processDelay(samples, wHead, feedback, numChannels, s, ts);
				s = ts;
			}
			else if (msg.isPitchWheel())
			{
				const auto ts = it.samplePosition;
				val.pb = (static_cast<double>(msg.getPitchWheelValue()) * PBInv - .5) * xenManager.getPitchbendRange() * 2.;
				updatePitch(numChannels);
				processDelay(samples, wHead, feedback, numChannels, s, ts);
				s = ts;
			}
		}
	}

	void CombFilter::processDelay(double** samples, const int* wHead, double feedback, int numChannels, int startIdx, int endIdx) noexcept
	{
		const auto numSamples = endIdx - startIdx;
		const auto smoothInfo = smooth(curDelay, numSamples);
		if (smoothInfo.smoothing)
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

	void CombFilter::updatePitch(int numChannels) noexcept
	{
		curDelay = val.getDelaySamples(xenManager, Fs);
		delay.setDampFreqHz(val.freqHz, numChannels);
	}
}