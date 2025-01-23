#include "Comb.h"

namespace dsp
{
	namespace hnm
	{
		// Val

		Val::Val() :
			freqHz(0.),
			pitchNote(0.),
			pitchParam(0.),
			pb(0.),
			delaySamples(0.)
		{}

		double Val::getDelaySamples(const XenManager& xen, double Fs) noexcept
		{
			freqHz = xen.noteToFreqHzWithWrap(pitchNote + pitchParam + pb, LowestFrequencyHz);
			delaySamples = math::freqHzToSamples(freqHz, Fs);
			return delaySamples;
		}

		double foldFc(double x) noexcept
		{
			while (x >= .5)
			{
				x = 1. - x;
				if (x < 0.)
					x = -x;
			}
			return x;
		}

		// DelayFeedback

		DelayFeedback::DelayFeedback() :
			ringBuffer(),
			size(0),
			sleepyTimer(0),
			sleepyThreshold(0),
			ringing(false)
		{
		}

		void DelayFeedback::prepare(double sampleRate, int _size)
		{
			size = _size;
			ringBuffer.setSize(2, size, false, true, false);

			sleepyTimer = 0;
			sleepyThreshold = static_cast<int>(sampleRate / 16.);
			ringing = false;
		}

		void DelayFeedback::operator()(double** samples, const int* wHead, const double* rHead,
			const double* fbBuffer, int numSamples, int ch) noexcept
		{
			auto ringBuf = ringBuffer.getArrayOfWritePointers();

			auto smpls = samples[ch];
			auto ring = ringBuf[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto w = wHead[s];
				const auto r = rHead[s];
				const auto fb = fbBuffer[s];

				const auto smplPresent = smpls[s];
				const auto smplDelayed = math::cubicHermiteSpline(ring, r, size);

				const auto sOut = math::tanhApprox(smplDelayed * fb) + smplPresent;
				const auto sIn = sOut;

				ring[w] = sIn;
				smpls[s] = sOut;

				static constexpr double Eps = 1e-3;
				const auto sOutAbs = sOut * sOut;
				if (sOutAbs > Eps)
				{
					ringing = true;
					sleepyTimer = 0;
				}
			}

			sleepyTimer += numSamples;
			ringing = sleepyTimer < sleepyThreshold;
		}

		bool DelayFeedback::isRinging() const noexcept
		{
			return ringing;
		}

		// ReadHead

		ReadHead::ReadHead() :
			size(0.)
		{}

		void ReadHead::prepare(double _size)
		{
			size = _size;
		}

		void ReadHead::operator()(double* delayBuf,
			const int* wHead, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto w = static_cast<double>(wHead[s]);
				auto r = w - delayBuf[s];
				if (r < 0.)
					r += size;
				delayBuf[s] = r;
			}
		}

		// Voice

		Voice::Voice() :
			delayPRMs{ 0.,0. },
			feedbackPRMs{ 0., 0. },
			feedbackInfos(),
			readHead(),
			delay(),
			vals(),
			Fs(0.),
			size(0)
		{
		}

		void Voice::prepare(double sampleRate)
		{
			Fs = sampleRate;
			for (auto& delayPRM : delayPRMs)
				delayPRM.prepare(Fs, 3.);
			for (auto& feedbackPRM : feedbackPRMs)
				feedbackPRM.prepare(sampleRate, 1.);
			const auto sizeD = std::ceil(math::freqHzToSamples(LowestFrequencyHz, Fs));
			size = static_cast<int>(sizeD);
			readHead.prepare(sizeD);
			delay.prepare(sampleRate, size);
			for (auto& val : vals)
				val.delaySamples = 0.;
		}

		void Voice::operator()(double** samples, const MidiBuffer& midi,
			const int* wHead, const XenManager& xenManager,
			double retune, double retuneWidth,
			double fb, double fbEnv, double fbWidth, double envGenMod,
			int numChannels, int numSamples) noexcept
		{
			updateRetuneParams(xenManager, retune, retuneWidth, numChannels);
			updateFeedbackParams(fb, fbWidth, fbEnv, envGenMod, numChannels, numSamples);
			processMIDI(midi, xenManager, numChannels, numSamples);

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto feedbackBuffer = feedbackInfos[ch].buf;
				auto delayBuffer = delayPRMs[ch].buf.data();

				readHead
				(
					delayBuffer,
					wHead,
					numSamples
				);


				delay
				(
					samples,
					wHead,
					delayBuffer,
					feedbackBuffer,
					numSamples,
					ch
				);
			}
		}

		bool Voice::isRinging() const noexcept
		{
			return delay.isRinging();
		}

		void Voice::updateRetuneParams(const XenManager& xenManager,
			double retune, double retuneWidth, int numChannels) noexcept
		{
			const auto retuneWidthHalf = retuneWidth * .5;
			double retunes[2] =
			{
				retune + retuneWidthHalf,
				retune - retuneWidthHalf
			};

			for (auto ch = 0; ch < numChannels; ++ch)
				if (vals[ch].pitchParam != retunes[ch])
				{
					vals[ch].pitchParam = retunes[ch];
					updatePitch(xenManager, ch);
				}
		}

		void Voice::updateFeedbackParams(double fb, double fbWidth,
			double fbEnv, double envGenMod,
			int numChannels, int numSamples) noexcept
		{
			const auto fbWidthHalf = fbWidth * .5;
			const double fbWidths[2] =
			{
				fb - fbWidthHalf,
				fb + fbWidthHalf
			};

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto fbModulated = math::limit(-1., 1., fbWidths[ch] + envGenMod * fbEnv);
				const auto fbRemapped = math::sinApprox(fbModulated * PiHalf);
				feedbackInfos[ch] = feedbackPRMs[ch](fbRemapped, numSamples);
				feedbackInfos[ch].copyToBuffer(numSamples);
			}
		}

		void Voice::processMIDI(const MidiBuffer& midi,
			const XenManager& xenManager,
			int numChannels, int numSamples) noexcept
		{
			auto s = 0;

			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					const auto ts = it.samplePosition;
					const auto note = msg.getNoteNumber();
					const auto noteD = static_cast<double>(note);
					for (auto& val : vals)
						val.pitchNote = noteD;
					updatePitch(xenManager, numChannels, s, ts);
					s = ts;
				}
				else if (msg.isPitchWheel())
				{
					const auto ts = it.samplePosition;
					const auto pb = (static_cast<double>(msg.getPitchWheelValue()) * PBInv - .5) * xenManager.getPitchbendRange() * 2.;
					for (auto& val : vals)
						val.pb = pb;
					updatePitch(xenManager, numChannels, s, ts);
					s = ts;
				}
			}

			updatePitch(xenManager, numChannels, s, numSamples);
		}

		void Voice::updatePitch(const XenManager& xenManager, int numChannels, int s, int ts) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto curDelay = vals[ch].delaySamples;
				auto delayInfo = delayPRMs[ch](curDelay, s, ts);
				delayInfo.copyToBuffer(s, ts);
				updatePitch(xenManager, ch);
			}
		}

		void Voice::updatePitch(const XenManager& xenManager, int ch) noexcept
		{
			vals[ch].getDelaySamples(xenManager, Fs);
		}

		// Comb

		Comb::Comb() :
			wHead(),
			voices()
		{}

		void Comb::prepare(double sampleRate)
		{
			for (auto i = 0; i < voices.size(); ++i)
				voices[i].prepare(sampleRate);
			wHead.prepare(voices[0].size);
		}

		void Comb::operator()(int numSamples) noexcept
		{
			wHead(numSamples);
		}

		void Comb::operator()(double** samples, const MidiBuffer& midi,
			const arch::XenManager& xen,
			double retune, double retuneWidth,
			double fb, double fbEnv, double fbWidth,
			double envGenMod,
			int numChannels, int numSamples, int v) noexcept
		{
			voices[v]
			(
				samples, midi, wHead.data(), xen,
				retune, retuneWidth, fb, fbEnv, fbWidth, envGenMod,
				numChannels, numSamples
			);
		}

		bool Comb::isRinging(int v) const noexcept
		{
			return voices[v].isRinging();
		}
	}
}