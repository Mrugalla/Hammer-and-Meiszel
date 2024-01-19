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
#include "AutoGain.h"

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

		inline double calcBandwidthFc(double reso, double sampleRateInv) noexcept
		{
			const auto bw = 20. - math::tanhApprox(3. * reso) * 19.;
			const auto bwFc = bw * sampleRateInv;
			return bwFc;
		}

		struct DualMaterial
		{
			DualMaterial() :
				materials(),
				peakInfos()
			{
				materials[0].load(BinaryData::Eiskaffee_wav, BinaryData::Eiskaffee_wavSize);
				generateSaw(materials[1]);
				peakInfos = materials[0].peakInfos;
			}

			void setMix(float mix) noexcept
			{
				for (auto i = 0; i < NumFilters; ++i)
				{
					const auto mag0 = materials[0].peakInfos[i].mag;
					const auto mag1 = materials[1].peakInfos[i].mag;
					const auto mag = mag0 + mix * (mag1 - mag0);
					peakInfos[i].mag = mag;

					const auto ratio0 = materials[0].peakInfos[i].ratio;
					const auto ratio1 = materials[1].peakInfos[i].ratio;
					const auto ratio = ratio0 + mix * (ratio1 - ratio0);
					peakInfos[i].ratio = ratio;
				}
			}

			double getMag(int i) const noexcept
			{
				return static_cast<double>(peakInfos[i].mag);
			}

			double getRatio(int i) const noexcept
			{
				return static_cast<double>(peakInfos[i].ratio);
			}

		protected:
			std::array<Material, 2> materials;
			std::array<Material::PeakInfo, NumFilters> peakInfos;
		};

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

					Envelope();

					double operator()(bool) noexcept;

					double operator()() noexcept;

					double env, atk, rls;
					State state;
					bool noteOn;
				protected:

					double processAttack() noexcept;

					double processRelease() noexcept;
				};

				struct Filter
				{
					Filter(const arch::XenManager&, const DualMaterial&);

					void prepare(double);

					/* samples, midi, numChannels, numSamples */
					void operator()(double**, const MidiBuffer&, int, int) noexcept;

					void setFrequencyHz(double) noexcept;

					void updateFreqRatios() noexcept;

					/* bw */
					void setBandwidth(double) noexcept;

				private:
					const arch::XenManager& xen;
					const DualMaterial& material;
					std::array<ResonatorStereo<Resonator2>, NumFilters> resonators;
					double freqHz, sampleRate, nyquist;
					int numFiltersBelowNyquist;

					/* samples, numChannels, startIdx, endIdx */
					void applyFilter(double**, int, int, int) noexcept;
				};

				Voice(const arch::XenManager&, const DualMaterial&);

				void prepare(double);

				/* bw */
				void setBandwidth(double) noexcept;

				/* samplesSrc, samplesDest, midi, numChannels, numSamples */
				void operator()(const double**, double**, const MidiBuffer&, int, int) noexcept;

				Filter filter;
				Envelope env;
				std::array<double, BlockSize2x> envBuffer;
				double gain;
				bool sleepy;
			protected:
				/* samplesSrc, samplesDest, midi, numChannels, numSamples */
				void processBlock(const double**, double**, const MidiBuffer&, int, int) noexcept;

				/* midi, numSamples */
				void synthesizeEnvelope(const MidiBuffer&, int) noexcept;

				/* s, ts */
				int synthesizeEnvelope(int, int) noexcept;

				/* samplesSrc, samplesDest, numChannels, numSamples */
				void processEnvelope(const double**, double**, int, int) noexcept;

				/* samplesDest, numChannels, numSamples */
				void detectSleepy(double**, int, int) noexcept;

				/* samplesDest, numChannels, numSamples */
				bool bufferSilent(double* const*, int, int) const noexcept;
			};

			Filter(const arch::XenManager& xen) :
				autoGainReso(),
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
				initAutoGainReso();
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
				PPMIDIBand& parallelProcessor, double _modalMix, double _reso,
				int numChannels, int numSamples) noexcept
			{
				updateModalMix(_modalMix);
				updateReso(_reso);

				for (auto v = 0; v < dsp::NumMPEChannels; ++v)
					processVoice(samplesSrc, parallelProcessor, voiceSplit, v, numChannels, numSamples);
			}

			AutoGain5 autoGainReso;
			DualMaterial material;
			std::array<Voice, NumMPEChannels> voices;
			double modalMix, reso, sampleRateInv;
		private:
			void updateModalMix(double _modalMix) noexcept
			{
				if (modalMix == _modalMix)
					return;
				modalMix = _modalMix;
				material.setMix(static_cast<float>(modalMix));
				for (auto& voice : voices)
					voice.filter.updateFreqRatios();
			}

			void updateReso(double _reso) noexcept
			{
				if (reso == _reso)
					return;
				reso = _reso;
				autoGainReso.updateParameterValue(reso);
				const auto bwFc = calcBandwidthFc(reso, sampleRateInv);
				for (auto& voice : voices)
				{
					voice.setBandwidth(bwFc);
					voice.gain = autoGainReso();
				}
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

			void initAutoGainReso()
			{
				ResonatorStereo<Resonator2> resonator;
				resonator.setCutoffFc(500. / 44100.);
				resonator.setGain(1.);
				
				autoGainReso.prepareGains
				(
					[&](double* smpls, double _reso, int numSamples)
					{
						reso = _reso;
						const auto bwFc = calcBandwidthFc(reso, 1. / 44100.);
						resonator.setBandwidth(bwFc);
						resonator.update();
						resonator.reset();
						for (auto s = 0; s < numSamples; ++s)
						{
							const auto dry = smpls[s];
							const auto wet = resonator(dry, 0);
							smpls[s] = wet;
						}
					}
				);
			}
		};
	}
	
}

/*

todo:
	if importing sound can not produce NumFilters amount filters, discard or fallback method

performance:
	filter performance
		SIMD
		try copying filter state without recalcing coefficients
			reso
			freq

features:
	material
		mags
			saturate
		freqRatios
			harmonize (nearest integer)

*/