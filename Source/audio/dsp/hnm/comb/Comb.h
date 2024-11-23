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
				pb(0.)
			{}

			double getDelaySamples(const XenManager& xen, double Fs) noexcept
			{
				freqHz = xen.noteToFreqHzWithWrap(pitchNote + pitchParam + pb, LowestFrequencyHz);
				return math::freqHzToSamples(freqHz, Fs);
			}

			double freqHz, pitchNote, pitchParam, pb;
		};

		struct Parameters
		{
			Parameters() :
				feedback(-0.),
				fbRemapped(-0.)
			{
			}

			void prepare()
			{
				feedback = -0.;
				fbRemapped = 0.;
			}

			// feedback[-1,1]
			void updateFeedback(double _feedback) noexcept
			{
				if (feedback == _feedback)
					return;
				feedback = _feedback;
				fbRemapped = math::sinApprox(feedback * PiHalf);
			}

			double getFeedback() const noexcept
			{
				return fbRemapped;
			}
		protected:
			double feedback, fbRemapped;
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
			DelayFeedback(Parameters& _params) :
				params(_params),
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

			// samples, wHead, rHead, numChannels, numSamples
			void operator()(double** samples, const int* wHead, const double* rHead,
				int numChannels, int numSamples) noexcept
			{
				auto ringBuf = ringBuffer.getArrayOfWritePointers();
				const auto fb = params.getFeedback();

				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samples[ch];
					auto ring = ringBuf[ch];

					for (auto s = 0; s < numSamples; ++s)
					{
						const auto w = wHead[s];
						const auto r = rHead[s];

						const auto smplPresent = smpls[s];
						const auto smplDelayed = math::cubicHermiteSpline(ring, r, size);

						const auto sOut = math::tanhApprox(smplDelayed * fb) + smplPresent;
						const auto sIn = sOut;

						ring[w] = sIn;
						smpls[s] = sOut;
					}
				}
			}

		protected:
			Parameters& params;
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
			Voice(Parameters& _params) :
				params(_params),
				delayPRM(0.),
				//delayBuffer(),
				readHead(),
				delay(params),
				val(),
				Fs(0.), curDelay(0.),
				size(0)
			{
			}

			// sampleRate, size
			void prepare(double sampleRate)
			{
				Fs = sampleRate;
				const auto sizeD = std::ceil(math::freqHzToSamples(LowestFrequencyHz, Fs));
				size = static_cast<int>(sizeD);
				readHead.prepare(sizeD);
				delay.prepare(size);
				curDelay = 0.;
			}

			// samples, midi, wHead, xen, retune[-n,n]semi, numChannels, numSamples
			void operator()(double** samples, const MidiBuffer& midi,
				const int* wHead, const XenManager& xenManager, double retune,
				int numChannels, int numSamples) noexcept
			{
				if (val.pitchParam != retune)
				{
					val.pitchParam = retune;
					updatePitch(xenManager);
				}

				processMIDI(midi, xenManager, numSamples);
				
				auto delayBuf = delayPRM.buf.data();

				readHead
				(
					delayBuf,
					wHead,
					numSamples
				);
				
				delay
				(
					samples,
					wHead, delayBuf,
					numChannels, numSamples
				);
			}

		protected:
			Parameters& params;
			PRMD delayPRM;
			ReadHead readHead;
			DelayFeedback delay;
			Val val;
			double Fs, curDelay;
		public:
			int size;
		private:
			// midi, xenManager, numSamples
			void processMIDI(const MidiBuffer& midi,
				const XenManager& xenManager, int numSamples) noexcept
			{
				auto s = 0;

				for (const auto it : midi)
				{
					const auto msg = it.getMessage();
					if (msg.isNoteOn())
					{
						const auto ts = it.samplePosition;
						const auto note = msg.getNoteNumber();
						val.pitchNote = static_cast<double>(note);
						updatePitch(xenManager, s, ts);
						s = ts;
					}
					else if (msg.isPitchWheel())
					{
						const auto ts = it.samplePosition;
						val.pb = (static_cast<double>(msg.getPitchWheelValue()) * PBInv - .5) * xenManager.getPitchbendRange() * 2.;
						updatePitch(xenManager, s, ts);
						s = ts;
					}
				}

				updatePitch(xenManager, s, numSamples);
			}

			// xenManager, startIdx, endIdx
			void updatePitch(const XenManager& xenManager, int s, int ts) noexcept
			{
				auto delayInfo = delayPRM(curDelay, s, ts);
				delayInfo.copyToBuffer(s, ts);
				updatePitch(xenManager);
			}

			// xenManager
			void updatePitch(const XenManager& xenManager) noexcept
			{
				curDelay = val.getDelaySamples(xenManager, Fs);
			}
		};

		using VoicesArray = std::array<Voice, NumMPEChannels>;

		struct Comb
		{
			Comb() :
				params(),
				wHead(),
				voices
				{
					Voice(params), Voice(params), Voice(params), Voice(params), Voice(params),
					Voice(params), Voice(params), Voice(params), Voice(params), Voice(params),
					Voice(params), Voice(params), Voice(params), Voice(params), Voice(params)
				}
			{}

			// Fs
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

			// feedback[-1,1]
			void updateParameters(double feedback) noexcept
			{
				params.updateFeedback(feedback);
			}

			// samples, midi, xen, retune, numChannels, numSamples, voiceIdx
			void operator()(double** samples, const MidiBuffer& midi,
				const arch::XenManager& xen, double retune,
				int numChannels, int numSamples, int v) noexcept
			{
				voices[v]
				(
					samples, midi, wHead.data(), xen, retune,
					numChannels, numSamples
				);
			}

		protected:
			Parameters params;
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