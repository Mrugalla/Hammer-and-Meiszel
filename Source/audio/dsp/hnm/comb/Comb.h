#pragma once
#include "../../../../libs/MoogLadders-master/src/Filters.h"
#include "../../../../arch/XenManager.h"
#include "../../midi/MPESplit.h"
#include "../../ParallelProcessor.h"
#include "../../../../arch/Math.h"
#include "../../PRM.h"
#include "../../WHead.h"

namespace dsp
{
	namespace hnm
	{
		using XenManager = arch::XenManager;
		static constexpr int Slope = 4;
		using AllpassSlope = moog::BiquadFilterSlope<Slope>;
		using AllpassSlopeChannel = std::array<AllpassSlope, 2>;
		static constexpr double PB = 0x3fff;
		static constexpr double PBInv = 1. / PB;

		class Voice
		{
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

			template<size_t NumSequences, size_t SeriesSize>
			struct AllpassSeries
			{
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
				DelayFeedback() :
					ringBuffer(),
					allpassSeries(),
					sampleRateInv(1.),
					size(0)
				{
					allpassSeries.addSequence({ 1,2,3,4,5,6 }, 0);
					allpassSeries.addSequence({ 1,2,3,5,8,13 }, 1);
					allpassSeries.addSequence({ 1,2,4,8,16,32 }, 2);
				}

				// sampleRate, size
				void prepare(double sampleRate, int _size)
				{
					sampleRateInv = 1. / sampleRate;
					size = _size;
					ringBuffer.setSize(2, size, false, true, false);
					allpassSeries.prepare(sampleRateInv);
				}

				// dampFreqHz, sequencesPos, numChannels
				void setDampFreqHz(double dampFreqHz, double sequencesPos, int numChannels) noexcept
				{
					allpassSeries.setDampFreqHz(dampFreqHz, sequencesPos, numChannels);
				}

				// apReso, numChannels
				void setAPReso(double apReso, int numChannels) noexcept
				{
					allpassSeries.setAPReso(apReso, numChannels);
				}

				// samples, wHead, rHead, feedback[-1,1], apResoInfo, numChannels, startIdx, endIdx
				void operator()(double** samples, const int* wHead,
					const double* rHead, double fb, PRMInfoD apResoInfo, int numChannels, int startIdx, int endIdx) noexcept
				{
					auto ringBuf = ringBuffer.getArrayOfWritePointers();

					if (apResoInfo.smoothing)
					{
						for (auto ch = 0; ch < numChannels; ++ch)
						{
							auto smpls = samples[ch];
							auto ring = ringBuf[ch];

							for (auto s = startIdx; s < endIdx; ++s)
							{
								setAPReso(apResoInfo[s], numChannels);

								const auto w = wHead[s];
								const auto r = rHead[s];

								const auto smplPresent = smpls[s];
								const auto smplDelayed = math::cubicHermiteSpline(ring, r, size);

								const auto sOut = allpassSeries(smplDelayed, ch) * fb + smplPresent;
								const auto sIn = sOut;

								ring[w] = sIn;
								smpls[s] = sOut;
							}
						}
					}
					else
					{
						for (auto ch = 0; ch < numChannels; ++ch)
						{
							auto smpls = samples[ch];
							auto ring = ringBuf[ch];

							for (auto s = startIdx; s < endIdx; ++s)
							{
								const auto w = wHead[s];
								const auto r = rHead[s];

								const auto smplPresent = smpls[s];
								const auto smplDelayed = math::cubicHermiteSpline(ring, r, size);

								const auto sOut = allpassSeries(smplDelayed, ch) * fb + smplPresent;
								const auto sIn = sOut;

								ring[w] = sIn;
								smpls[s] = sOut;
							}
						}
					}
				}

			protected:
				AudioBuffer ringBuffer;
				AllpassSeries<3, 6> allpassSeries;
				double sampleRateInv;
				int size;
			};

			static constexpr double LowestFrequencyHz = 20.;

		public:
			Voice() :
				smooth(0.),
				readHead(),
				apResoPRM(8.),
				delay(),
				val(),
				Fs(0.), sizeF(0.), curDelay(0.), sequencesPos(0.),
				size(0)
			{
			}

			// sampleRate
			void prepare(double sampleRate)
			{
				Fs = sampleRate;
				sizeF = std::ceil(math::freqHzToSamples(LowestFrequencyHz, Fs));
				size = static_cast<int>(sizeF);
				delay.prepare(Fs, size);
				smooth.prepare(Fs, 1.);
				curDelay = 0.;
				apResoPRM.prepare(sampleRate, 4.);
			}

			void updateParameters(const XenManager& xenManager, double _sequencesPos, int numChannels) noexcept
			{
				sequencesPos = _sequencesPos;
				updatePitch(xenManager, numChannels);
			}

			// samples, midi, wHead, feedback [-1,1], retune [-n,n]semi, apReso[.1, 8], numChannels, numSamples
			void operator()(double** samples, const MidiBuffer& midi,
				const int* wHead, const XenManager& xenManager, double feedback, double retune, double apReso,
				int numChannels, int numSamples) noexcept
			{
				if (val.pitchParam != retune)
				{
					val.pitchParam = retune;
					updatePitch(xenManager, numChannels);
				}

				const auto apResoInfo = apResoPRM(apReso, numSamples);

				auto s = 0;
				processMIDI(samples, midi, wHead, xenManager, apResoInfo, feedback, numChannels, s);
				processDelay(samples, wHead, apResoInfo, feedback, numChannels, s, numSamples);
			}

		protected:
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
			void processMIDI(double** samples, const MidiBuffer& midi, const int* wHead,
				const XenManager& xenManager, PRMInfoD apResoInfo, double feedback, int numChannels, int& s) noexcept
			{
				for (const auto it : midi)
				{
					const auto msg = it.getMessage();
					if (msg.isNoteOn())
					{
						const auto ts = it.samplePosition;
						const auto note = msg.getNoteNumber();
						val.pitchNote = static_cast<double>(note);
						updatePitch(xenManager, numChannels);
						processDelay(samples, wHead, apResoInfo, feedback, numChannels, s, ts);
						s = ts;
					}
					else if (msg.isPitchWheel())
					{
						const auto ts = it.samplePosition;
						val.pb = (static_cast<double>(msg.getPitchWheelValue()) * PBInv - .5) * xenManager.getPitchbendRange() * 2.;
						updatePitch(xenManager, numChannels);
						processDelay(samples, wHead, apResoInfo, feedback, numChannels, s, ts);
						s = ts;
					}
				}
			}

			// samples, wHead, apResoInfo, feedback, numChannels, startIdx, endIdx
			void processDelay(double** samples, const int* wHead,
				PRMInfoD apResoInfo, double feedback,
				int numChannels, int startIdx, int endIdx) noexcept
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
					feedback, apResoInfo,
					numChannels, startIdx, endIdx
				);
			}

			// xenManager, numChannels
			void updatePitch(const XenManager& xenManager, int numChannels) noexcept
			{
				curDelay = val.getDelaySamples(xenManager, Fs);
				delay.setDampFreqHz(val.freqHz, sequencesPos, numChannels);
			}
		};

		struct Comb
		{
			Comb() :
				wHead(),
				voices(),
				feedback(0.),
				fbRemapped(0.),
				sequencesPos(-1.)
			{}

			// Fs
			void prepare(double sampleRate)
			{
				for (auto i = 0; i < voices.size(); ++i)
					voices[i].prepare(sampleRate);
				wHead.prepare(voices[0].size);
			}

			// numSamples
			void synthesizeWHead(int numSamples) noexcept
			{
				wHead(numSamples);
			}

			void updateParameters(const arch::XenManager& xen, double _feedback, double _sequencesPos, int numChannels) noexcept
			{
				if (feedback != _feedback)
				{
					feedback = _feedback;
					fbRemapped = math::sinApprox(_feedback * PiHalf);
				}
				if(sequencesPos != _sequencesPos)
				{
					sequencesPos = _sequencesPos;
					for (auto i = 0; i < voices.size(); ++i)
						voices[i].updateParameters(xen, sequencesPos, numChannels);
				}
			}

			// samples, midi, retune, numChannels, numSamples, voiceIdx
			void operator()(double** samples, const MidiBuffer& midi,
				const arch::XenManager& xen, double retune, double apReso,
				int numChannels, int numSamples, int v) noexcept
			{
				voices[v]
				(
					samples, midi, wHead.data(), xen,
					fbRemapped, retune, apReso,
					numChannels, numSamples
				);
			}

		protected:
			WHead1x wHead;
			std::array<Voice, NumMPEChannels> voices;
			double feedback, fbRemapped, sequencesPos;
		};
	}
}

// todo
// 
//params
	//damp[20, 20k]
	//chord type[+5, +7, +12, +17, +19, +24]
	//chord depth[0, 1]
	//dry gain[-inf, 0]
	//wet gain[-inf, 0]
	//flanger/phaser switch