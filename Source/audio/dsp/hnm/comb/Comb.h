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
			Val() :
				freqHz(0.),
				pitchNote(0.),
				pitchParam(0.),
				pb(0.),
				delaySamples(0.)
			{}

			double getDelaySamples(const XenManager& xen, double Fs) noexcept
			{
				freqHz = xen.noteToFreqHzWithWrap(pitchNote + pitchParam + pb, LowestFrequencyHz);
				delaySamples = math::freqHzToSamples(freqHz, Fs);
				return delaySamples;
			}

			double freqHz, pitchNote, pitchParam, pb, delaySamples;
		};

		inline double foldFc(double x) noexcept
		{
			while (x >= .5)
			{
				x = 1. - x;
				if (x < 0.)
					x = -x;
			}
			return x;
		}

		struct DelayFeedback
		{
			DelayFeedback() :
				ringBuffer(),
				size(0)
			{
			}

			// delaySize
			void prepare(int _size)
			{
				size = _size;
				ringBuffer.setSize(2, size, false, true, false);
			}

			// samples, wHead, rHead, feedbackBuffer, numSamples, ch
			void operator()(double** samples, const int* wHead, const double* rHead,
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

		protected:
			AudioBuffer ringBuffer;
			int size;
		};

		struct ReadHead
		{
			ReadHead() :
				size(0.)
			{}

			void prepare(double _size)
			{
				size = _size;
			}

			// delayBuf, wHead, numSamples
			void operator()(double* delayBuf,
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
		protected:
			double size;
		};

		struct Voice
		{
			Voice() :
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

			// sampleRate, size
			void prepare(double sampleRate)
			{
				Fs = sampleRate;
				for (auto& delayPRM : delayPRMs)
					delayPRM.prepare(Fs, 3.);
				for (auto& feedbackPRM : feedbackPRMs)
					feedbackPRM.prepare(sampleRate, 1.);
				const auto sizeD = std::ceil(math::freqHzToSamples(LowestFrequencyHz, Fs));
				size = static_cast<int>(sizeD);
				readHead.prepare(sizeD);
				delay.prepare(size);
				for (auto& val : vals)
					val.delaySamples = 0.;
			}

			// samples, midi, wHead, xen,
			// retune[-n,n]semi, retuneWidth[-1,1],
			// feedback[-1,1], feedbackEnv[-2,2], feedbackWidth[-2,2],
			// envGenMod[-1,1], numChannels, numSamples
			void operator()(double** samples, const MidiBuffer& midi,
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

		protected:
			std::array<PRMD, 2> delayPRMs, feedbackPRMs;
			std::array<PRMInfoD, 2> feedbackInfos;
			ReadHead readHead;
			DelayFeedback delay;
			std::array<Val, 2> vals;
			double Fs;
		public:
			int size;
		private:
			// xen, retune, retuneWidth, numChannels
			void updateRetuneParams(const XenManager& xenManager,
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

			// fb, fbWidth, fbEnv, envGenMod, numChannels, numSamples
			void updateFeedbackParams(double fb, double fbWidth,
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

			// midi, xenManager, numChannels, numSamples
			void processMIDI(const MidiBuffer& midi,
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
						for(auto& val: vals)
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

			// xenManager, numChannels, startIdx, endIdx
			void updatePitch(const XenManager& xenManager, int numChannels, int s, int ts) noexcept
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto curDelay = vals[ch].delaySamples;
					auto delayInfo = delayPRMs[ch](curDelay, s, ts);
					delayInfo.copyToBuffer(s, ts);
					updatePitch(xenManager, ch);
				}
			}

			// xenManager, ch
			void updatePitch(const XenManager& xenManager, int ch) noexcept
			{
				vals[ch].getDelaySamples(xenManager, Fs);
			}
		};

		using VoicesArray = std::array<Voice, NumMPEChannels>;

		struct Comb
		{
			Comb() :
				wHead(),
				voices()
			{}

			// sampleRate
			void prepare(double sampleRate)
			{
				for (auto i = 0; i < voices.size(); ++i)
					voices[i].prepare(sampleRate);
				wHead.prepare(voices[0].size);
			}

			// numSamples
			void operator()(int numSamples) noexcept
			{
				wHead(numSamples);
			}

			// samples, midi, xen, retune, feedback, feedbackEnv, feedbackWidth,
			// numChannels, numSamples, voiceIdx
			void operator()(double** samples, const MidiBuffer& midi,
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