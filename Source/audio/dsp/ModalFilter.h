#pragma once
#include "../../arch/XenManager.h"
#include "../../libs/MoogLadders-master/src/Filters.h"
#include "../../arch/Math.h"
#include "PRM.h"
#include "Convolver.h"
#include "ParallelProcessor.h"
#include "midi/MPESplit.h"
#include <BinaryData.h>
#include <juce_dsp/juce_dsp.h>
#include "Resonator.h"

namespace dsp
{
	namespace modal
	{
#if JUCE_DEBUG
		static constexpr int NumFilters = 12;
#else
		static constexpr int NumFilters = 48;
#endif

		struct Material
		{
			static constexpr int FFTOrder = 15;
			static constexpr int FFTSize = 1 << FFTOrder;
			static constexpr int FFTSize2 = FFTSize * 2;
			static constexpr float FFTSizeF = static_cast<float>(FFTSize);
			static constexpr float FFTSizeInv = 1.f / FFTSizeF;
			static constexpr int FFTSizeHalf = FFTSize / 2;
			using MaterialBuffer = std::array<float, FFTSize>;
			using FFT = juce::dsp::FFT;

			struct PeakInfo
			{
				float mag;
				float ratio;
			};

			Material();

			// data, size
			void load(const char*, int);

			void load();

			MaterialBuffer buffer;
			std::array<PeakInfo, NumFilters> peakInfos;
			std::atomic<bool> updated;
			float sampleRate;
			int numFilters;
		private:
			struct PeakIndexInfo
			{
				std::vector<int> indexes;
			};

			// data, size
			void fillBuffer(const char*, int);

			// fifo
			void applyFFT(float*);

			// bins
			void generateMagnitudes(float*) noexcept;

			// bins
			void normalize(float*) noexcept;

			// dest, src, cutoff, bw
			void applyLowpassFIR(float*, const float*, float, float);

			// dest, src, cutoff, reso, slope
			void applyLowpassIIR(float*, const float*, float, float, int = 1);

			// bins offset
			void applyNegativeDelay(float*, int) noexcept;

			// dest, a, b
			void divide(float*, const float*, const float*) noexcept;

			// bins, lowerLimitIdx, upperLimitidx
			int getMaxMagnitudeIdx(const float*, int, int) const noexcept;

			// bins, lowerLimitIdx, upperLimitidx
			int getMinMagnitudeIdx(const float*, int, int) const noexcept;

			// peakGroups, binsNorm, minBinidx
			float generatePeakGroups(std::vector<PeakIndexInfo>&, const float*, int);

			// peakGroups, binsNorm, targetThresholdDb, minBinIdx
			void applyAdaptiveThreshold(std::vector<PeakIndexInfo>&, const float*, float, int);

			// peakIndexes, bins, peakIdxInfos
			void generatePeakIndexes(std::vector<int>&, const float*,
				const std::vector<PeakIndexInfo>&);

			// fifo, peakIndexes
			float getHarm0Idx(const float*, const int*) const noexcept;

			// fifo, peakIndexes
			int getMinPeakIdx(const float*, const int*) const noexcept;

			// fifo, peakIndexes, harm0Idx
			void generatePeakInfos(const float*, const int*, float) noexcept;

			// name, buf, thresholdDb, peakIndexes, isLog
			void drawSpectrum(String&&, const float*, float,
				const int*, bool);
		};

		inline void generateSaw(Material& material)
		{
			material.sampleRate = static_cast<float>(material.buffer.size());
			for (auto s = 0; s < material.buffer.size(); ++s)
			{
				const auto sF = static_cast<float>(s);
				auto x = static_cast<float>(2 * NumFilters) * sF / material.sampleRate;
				x = x - std::floor(x);
				material.buffer[s] = 2.f * x - 1.f;
			}
			material.load();
		}

		inline void generateSquare(Material& material)
		{
			material.sampleRate = static_cast<float>(material.buffer.size());
			for (auto s = 0; s < material.buffer.size(); ++s)
			{
				const auto sF = static_cast<float>(s);
				auto x = static_cast<float>(2 * NumFilters) * sF / material.sampleRate;
				x = x - std::floor(x);
				material.buffer[s] = x < .5f ? 1.f : -1.f;
			}
			material.load();
		}

		struct Filter
		{
			struct Voice
			{
				struct Envelope
				{
					enum class State
					{
						Attack,
						Release
					};

					Envelope() :
						env(0.),
						atk(0.),
						rls(0.),
						state(State::Release),
						noteOn(false)
					{}

					double operator()(bool _noteOn) noexcept
					{
						noteOn = _noteOn;
						return operator()();
					}

					double operator()() noexcept
					{
						switch (state)
						{
						case State::Attack: return processAttack();
						case State::Release: return processRelease();
						default: return env;
						}
					}

					double env, atk, rls;
					State state;
					bool noteOn;
				protected:

					double processAttack() noexcept
					{
						if (noteOn)
						{
							env += atk * (1. - env);
							if (1. - env < 1e-6)
								env = 1.;
							return env;
						}
						state = State::Release;
						return processRelease();
					}

					double processRelease() noexcept
					{
						if (noteOn)
						{
							state = State::Attack;
							return processAttack();
						}
						env += rls * (0. - env);
						if (env < 1e-6)
							env = 0.;
						return env;
					}
				};

				struct Filter
				{
					Filter(const arch::XenManager& _xen, const Material& _material) :
						xen(_xen),
						material(_material),
						resonators(),
						sampleRate(1.),
						nyquist(.5),
						numFiltersBelowNyquist(0)
					{
					}

					void prepare(double _sampleRate)
					{
						sampleRate = _sampleRate;
						nyquist = sampleRate * .5;
						setFrequencyHz(1000.);
					}

					/* samples, midi, numChannels, numSamples */
					void operator()(double** samples, const MidiBuffer& midi,
						int numChannels, int numSamples) noexcept
					{
						auto s = 0;
						for (const auto it : midi)
						{
							const auto msg = it.getMessage();
							if (msg.isNoteOn())
							{
								const auto ts = it.samplePosition;
								const auto pitch = static_cast<double>(msg.getNoteNumber());
								const auto freq = xen.noteToFreqHzWithWrap(pitch);
								setFrequencyHz(freq);
								applyFilter(samples, numChannels, s, ts);
								s = ts;
							}
						}

						applyFilter(samples, numChannels, s, numSamples);
					}

					void setFrequencyHz(double freq) noexcept
					{
						for (auto i = 0; i < material.numFilters; ++i)
						{
							const auto& peakInfo = material.peakInfos[i];
							const auto freqRatio = peakInfo.ratio;
							const auto freqFilter = freq * freqRatio;
							if (freqFilter < nyquist)
							{
								const auto fc = math::freqHzToFc(freqFilter, sampleRate);
								auto& resonator = resonators[i];
								resonator.setCutoffFc(fc);
								resonator.reset();
								resonator.update();
							}
							else
							{
								numFiltersBelowNyquist = i;
								return;
							}
						}
						numFiltersBelowNyquist = material.numFilters;
					}

					void setBandwidth(double bw) noexcept
					{
						for (auto i = 0; i < material.numFilters; ++i)
						{
							auto& resonator = resonators[i];
							resonator.setBandwidth(bw);
							resonator.update();
						}
					}

				private:
					const arch::XenManager& xen;
					const Material& material;
					std::array<ResonatorStereo<Resonator2>, NumFilters> resonators;
					double sampleRate, nyquist;
					int numFiltersBelowNyquist;

					void applyFilter(double** samples, int numChannels,
						int startIdx, int endIdx) noexcept
					{
						const auto& peakInfos = material.peakInfos;

						for (auto ch = 0; ch < numChannels; ++ch)
						{
							auto smpls = samples[ch];
							for (auto i = startIdx; i < endIdx; ++i)
							{
								const auto dry = smpls[i];
								auto wet = 0.;
								for (auto f = 0; f < numFiltersBelowNyquist; ++f)
									wet += resonators[f](dry, ch) * peakInfos[f].mag;
								smpls[i] = wet;
							}
						}
					}
				};

				Voice(const arch::XenManager& xen, const Material& material) :
					filter(xen, material),
					env(),
					envBuffer(),
					sleepy(true)
				{}

				void prepare(double sampleRate)
				{
					filter.prepare(sampleRate);
					env.atk = math::msToInc(2., sampleRate);
					env.rls = math::msToInc(10., sampleRate);
				}

				void setBandwidth(double bw) noexcept
				{
					filter.setBandwidth(bw);
				}

				void operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
					int numChannels, int numSamples) noexcept
				{
					if (midi.isEmpty() && sleepy)
						return;
					processBlock(samplesSrc, samplesDest, midi, numChannels, numSamples);
					detectSleepy(samplesDest, numChannels, numSamples);
				}

				Filter filter;
				Envelope env;
				std::array<double, BlockSize2x> envBuffer;
				bool sleepy;
			protected:
				void processBlock(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
					int numChannels, int numSamples) noexcept
				{
					synthesizeEnvelope(midi, numSamples);
					processEnvelope(samplesSrc, samplesDest, numChannels, numSamples);
					filter(samplesDest, midi, numChannels, numSamples);
				}

				void synthesizeEnvelope(const MidiBuffer& midi, int numSamples) noexcept
				{
					auto s = 0;
					for (const auto it : midi)
					{
						const auto msg = it.getMessage();
						const auto ts = it.samplePosition;
						if (msg.isNoteOn())
						{
							s = synthesizeEnvelope(s, ts);
							env.noteOn = true;
						}
						else if (msg.isNoteOff())
						{
							s = synthesizeEnvelope(s, ts);
							env.noteOn = false;
						}
						else
							s = synthesizeEnvelope(s, ts);
					}

					synthesizeEnvelope(s, numSamples);
				}

				int synthesizeEnvelope(int s, int ts) noexcept
				{
					while (s < ts)
					{
						envBuffer[s] = env();
						++s;
					}
					return s;
				}

				void processEnvelope(const double** samplesSrc, double** samplesDest,
					int numChannels, int numSamples) noexcept
				{
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						const auto smplsSrc = samplesSrc[ch];
						auto smplsDest = samplesDest[ch];

						SIMD::multiply(smplsDest, smplsSrc, envBuffer.data(), numSamples);
					}
				}

				void detectSleepy(double** samplesDest, int numChannels, int numSamples) noexcept
				{
					if(env.state != Envelope::State::Release)
					{
						sleepy = false;
						return;
					}

					const bool bufferTooSmall = numSamples != BlockSize;
					if (bufferTooSmall)
						return;

					sleepy = bufferSilent(samplesDest, numChannels, numSamples);
					if (!sleepy)
						return;

					for (auto ch = 0; ch < numChannels; ++ch)
						SIMD::clear(samplesDest[ch], numSamples);
				}

				bool bufferSilent(double* const* samplesDest, int numChannels, int numSamples) const noexcept
				{
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto smpls = samplesDest[ch];
						for (auto s = 0; s < numSamples; ++s)
						{
							auto smpl = std::abs(smpls[s]);
							if (smpl > 1e-6)
								return false;
						}
					}
					return true;
				}
			};

			Filter(const arch::XenManager& xen) :
				material(),
				voices
				{
					Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
					Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
					Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material)
				},
				reso(-1.),
				sampleRateInv(1.)
			{
				material.load(BinaryData::Eiskaffee_wav, BinaryData::Eiskaffee_wavSize);
			}

			void prepare(double sampleRate)
			{
				for (auto& voice : voices)
					voice.prepare(sampleRate);
				reso = -1.;
				sampleRateInv = 1. / sampleRate;
			}

			/* samples, voiceSplit, parallelProcessor, reso]0,1], numChannels, numSamples */
			void operator()(const double** samplesSrc, const MPESplit& voiceSplit,
				PPMIDIBand& parallelProcessor, double _reso,
				int numChannels, int numSamples) noexcept
			{
				updateReso(_reso);

				for (auto v = 0; v < dsp::NumMPEChannels; ++v)
					processVoice(samplesSrc, parallelProcessor, voiceSplit, v, numChannels, numSamples);
			}

			Material material;
			std::array<Voice, NumMPEChannels> voices;
			double reso, sampleRateInv;
		private:
			void updateReso(double _reso) noexcept
			{
				if (reso == _reso)
					return;
				reso = _reso;
				const auto bw = 20. - reso * 19.;
				const auto bwFc = bw * sampleRateInv;
				for (auto& voice : voices)
					voice.setBandwidth(bwFc);
			}

			void processVoice(const double** samplesSrc, PPMIDIBand& parallelProcessor, const MPESplit& voiceSplit, int v,
				int numChannels, int numSamples) noexcept
			{
				const auto vVoice = v + 2;
				const auto& vMIDI = voiceSplit[vVoice];
				auto band = parallelProcessor[v];
				double* vSamples[] = { band.l, band.r };
				voices[v](samplesSrc, vSamples, vMIDI, numChannels, numSamples);
			}
		};
	}
	
}

/*

todo:
	gain compensate different resonance values

performance:
	filter performance
		SIMD
		try copying filter state without recalcing coefficients
			reso
			freq

features:
	interpolate between 2 materials
	saturate the mags with a parameter (lookuptable)

bugs:
	oversampling doubles the volume

*/