#include "Comb.h"

namespace dsp
{
	namespace hnm
	{
		// Params

		// Val

		Val::Val() :
			freqHz(0.),
			pitchNote(0.),
			pitchParam(-1.),
			pb(0.),
			delaySamples(0.)
		{}

		void Val::reset() noexcept
		{
			freqHz = 0.;
			pitchNote = -1.;
			pitchParam = 0.;
			pb = 0.;
			delaySamples = 0.;
		}

		void Val::updateDelaySamples(const XenManager& xen, double Fs) noexcept
		{
			const auto pbRange = xen.getPitchbendRange();
			freqHz = xen.noteToFreqHzWithWrap(pitchNote + pitchParam + pb * pbRange, LowestFrequencyHz);
			delaySamples = math::freqHzToSamples(freqHz, Fs);
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
			size(0)
		{
		}

		void DelayFeedback::prepare(int _size)
		{
			size = _size;
			ringBuffer.setSize(2, size, false, true, false);
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
			}
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
			wHead(),
			delayPRMs{ 0.,0. },
			feedbackPRMs{ 0., 0. },
			readHead(),
			delay(),
			vals(),
			Fs(0.),
			xenInfo(),
			sleepy(),
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
			wHead.prepare(size);
			readHead.prepare(sizeD);
			delay.prepare(size);
			for (auto& val : vals)
				val.reset();
			sleepy.prepare(sampleRate);
			xenInfo = XenManager::Info();
		}

		void Voice::operator()(double** samples,
			const XenManager& xenManager, const Params& params,
			double envGenMod, int numChannels, int numSamples) noexcept
		{
			updateParams(xenManager, params, envGenMod, numChannels, numSamples);
			applyDelay(samples, numChannels, numSamples);
			sleepy(samples, numChannels, numSamples);
		}

		void Voice::applyDelay(double** samples, int numChannels, int numSamples) noexcept
		{
			wHead(numSamples);
			const auto wHeadData = wHead.data();
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto& delayPRM = delayPRMs[ch];
				auto& val = vals[ch];
				auto info = delayPRM(val.delaySamples, numSamples);
				info.copyToBuffer(numSamples);

				const auto fbBuffer = feedbackPRMs[ch].buf.data();
				auto delayBuffer = delayPRM.buf.data();

				readHead
				(
					delayBuffer,
					wHeadData,
					numSamples
				);

				delay
				(
					samples,
					wHeadData,
					delayBuffer,
					fbBuffer,
					numSamples,
					ch
				);
			}
		}

		void Voice::triggerXen(const XenManager& xen, int numChannels) noexcept
		{
			xenInfo = xen.getInfo();
			updatePitch(xen, numChannels);
		}

		void Voice::triggerNoteOn(const XenManager& xen, double noteNumber, int numChannels) noexcept
		{
			for (auto& val : vals)
				val.pitchNote = noteNumber;
			updatePitch(xen, numChannels);
			sleepy.triggerNoteOn();
		}

		void Voice::triggerNoteOff() noexcept
		{
			sleepy.triggerNoteOff();
		}

		void Voice::triggerPitchbend(const XenManager& xen, double pitchbend, int numChannels) noexcept
		{
			for (auto& val : vals)
				val.pb = pitchbend;
			updatePitch(xen, numChannels);
		}

		bool Voice::isRinging() const noexcept
		{
			return sleepy.isRinging();
		}

		void Voice::updateParams(const XenManager& xen, const Params& params,
			double envGenMod, int numChannels, int numSamples) noexcept
		{
			bool wannaUpdate = false;

			const auto retuneWidthHalf = params.retuneWidth * .5;
			double retunes[2] =
			{
				params.retune + retuneWidthHalf,
				params.retune - retuneWidthHalf
			};

			for (auto ch = 0; ch < numChannels; ++ch)
				if (vals[ch].pitchParam != retunes[ch])
				{
					vals[ch].pitchParam = retunes[ch];
					wannaUpdate = true;
				}

			if(wannaUpdate)
				updatePitch(xen, numChannels);
			
			const double fbWidths[2] =
			{
				params.fb - params.fbWidth,
				params.fb + params.fbWidth
			};
			const auto envGained = envGenMod * params.fbEnv;
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto fbModulated = math::limit(-.99, .99, fbWidths[ch] + envGained);
				const auto fbRemapped = math::sinApprox(fbModulated * PiHalf);
				auto info = feedbackPRMs[ch](fbRemapped, numSamples);
				info.copyToBuffer(numSamples);
			}
		}

		void Voice::updatePitch(const XenManager& xenManager, int numChannels) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto& val = vals[ch];
				val.updateDelaySamples(xenManager, Fs);
			}
		}

		// Comb

		Comb::Comb() :
			voices()
		{}

		void Comb::prepare(double sampleRate)
		{
			for (auto i = 0; i < voices.size(); ++i)
				voices[i].prepare(sampleRate);
		}

		void Comb::operator()(double** samples,
			const arch::XenManager& xenManager, const Params& params,
			double envGenMod, int numChannels, int numSamples, int v) noexcept
		{
			voices[v]
			(
				samples, xenManager,
				params, envGenMod,
				numChannels, numSamples
			);
		}

		void Comb::triggerXen(const arch::XenManager& xen, int numChannels) noexcept
		{
			for (auto& voice: voices)
				voice.triggerXen(xen, numChannels);
		}

		void Comb::triggerNoteOn(const arch::XenManager& xen, double noteNumber, int numChannels, int v) noexcept
		{
			voices[v].triggerNoteOn(xen, noteNumber, numChannels);
		}

		void Comb::triggerNoteOff(int v) noexcept
		{
			voices[v].triggerNoteOff();
		}

		void Comb::triggerPitchbend(const arch::XenManager& xen, double pitchbend, int numChannels, int v) noexcept
		{
			voices[v].triggerPitchbend(xen, pitchbend, numChannels);
		}

		bool Comb::isRinging(int v) const noexcept
		{
			return voices[v].isRinging();
		}
	}
}