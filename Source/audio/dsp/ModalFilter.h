#pragma once
#include "../../arch/XenManager.h"
#include "../../libs/MoogLadders-master/src/Filters.h"
#include "../../arch/Math.h"
#include "PRM.h"
#include "Convolver.h"
#include <BinaryData.h>
#include <juce_dsp/juce_dsp.h>

namespace dsp
{
	struct StereoFilter  
	{
		static constexpr double MinReso = 1.5;
		static constexpr double ResoRange = 1.2;
		static constexpr double MinResoGainDb = -2.;
		static constexpr double MaxResoGainDb = -18.;
		static constexpr int MaxSlope = 2;
		using Filters = std::array<moog::BiquadFilterSlope<MaxSlope>, 2>;
		using Type = typename moog::BiquadFilter::Type;

		StereoFilter() :
			filters(),
			cutoffFc(moog::BiquadFilter::DefaultCutoffFc),
			reso(moog::BiquadFilter::DefaultResonance),
			type(Type::BP),
			gain(1.)
		{
		}

		void prepare() noexcept
		{
			for (auto& filter : filters)
				filter.prepare();
		}

		void operator()(double** samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				filters[ch](smpls, numSamples);
				SIMD::multiply(smpls, gain, numSamples);
			}
		}

		double operator()(double sample, int ch) noexcept
		{
			return filters[ch](sample) * gain;
		}

		bool setCutoffFc(double fc) noexcept
		{
			if (cutoffFc == fc)
				return false;
			cutoffFc = fc;
			filters[0].setCutoffFc(cutoffFc);
			filters[1].copyFrom(filters[0]);
			return true;
		}

		bool setResonance(double q) noexcept
		{
			if (reso == q)
				return false;
			reso = q;
			const auto r = MinReso - reso * reso * ResoRange;
			for (auto& filter : filters)
				filter.setResonance(r);
			gain = math::dbToAmp(MaxResoGainDb + r * (MinResoGainDb - MaxResoGainDb));
			return true;
		}

		bool setType(Type _type) noexcept
		{
			if (type == _type)
				return false;
			type = _type;
			for (auto& filter : filters)
				filter.setType(type);
			return true;
		}

		void setSlope(int slope) noexcept
		{
			for (auto& filter : filters)
				filter.setSlope(slope);
		}

		void update() noexcept
		{
			filters[0].update();
			filters[1].copyFrom(filters[0]);
		}

	protected:
		Filters filters;
		double cutoffFc, reso;
		Type type;
		double gain;
	};

	struct ModalFilter
	{
		static constexpr int FFTOrder = 15;
		static constexpr int NumFilters = 64;

		static constexpr int FFTSize = 1 << FFTOrder;
		static constexpr int FFTSize2 = FFTSize * 2;
		static constexpr float FFTSizeF = static_cast<float>(FFTSize);
		static constexpr float FFTSizeInv = 1.f / FFTSizeF;
		static constexpr int FFTSizeHalf = FFTSize / 2;
		using MaterialBuffer = std::array<float, FFTSize>;
		//using FifoBuffer = std::array<float, FFTSize2>;

		class Material
		{
			using FFT = juce::dsp::FFT;

			struct PeakIndexInfo
			{
				std::vector<int> indexes;
			};

			struct PeakInfo
			{
				float mag;
				float ratio;
			};
		public:

			Material() :
				buffer(),
				updated(false),
				audioSampleRate(0.f)
			{
			}

			void load(const char* data, int size)
			{
				fillBuffer(data, size);
				load();
			}

			void load()
			{
				std::vector<float> fifo, binsNorm;
				fifo.resize(FFTSize2);
				applyFFT(fifo.data());
				auto bins = fifo.data();
				{
					std::vector<float> binsLP;
					binsLP.resize(FFTSize2);
					applyLowpassIIR(binsLP.data(), bins, 50.f, .2f, 3);
					binsNorm.resize(FFTSize2);
					divide(binsNorm.data(), bins, binsLP.data());
					normalize(binsNorm.data());
				}
				const auto harm0Idx = getMaxMagnitudeIdx(binsNorm.data(), 0, FFTSizeHalf);
				std::vector<PeakIndexInfo> peakGroups;
				generatePeakGroups(peakGroups, binsNorm.data(), harm0Idx, NumFilters);
				std::vector<int> peakIndexes;
				peakIndexes.resize(NumFilters);
				generatePeakIndexes(peakIndexes, bins, peakGroups);
				generatePeakInfos(bins, peakIndexes.data(), static_cast<float>(harm0Idx));
				updated.store(true);
			}

			MaterialBuffer buffer;
			std::array<PeakInfo, NumFilters> peakInfos;
			std::atomic<bool> updated;
			float audioSampleRate;
		protected:
			
			void fillBuffer(const char* data, int size)
			{
				AudioBufferF tmpBuffer;
				audioSampleRate = static_cast<float>(loadFromMemory(tmpBuffer, data, size));
				const auto samples = tmpBuffer.getArrayOfReadPointers();
				const auto numChannels = tmpBuffer.getNumChannels();
				const auto numSamples = tmpBuffer.getNumSamples();
				for(auto& b: buffer)
					b = 0.f;
				auto len = numSamples < FFTSize ? numSamples : FFTSize;
				for (auto s = 0; s < len; ++s)
					buffer[s] = samples[0][s];
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < len; ++s)
						buffer[s] += samples[ch][s];
				const auto gain = 1.f / static_cast<float>(numChannels);
				SIMD::multiply(buffer.data(), gain, len);
			}

			void applyFFT(float* fifo)
			{
				FFT fft(FFTOrder);
				const auto windowFunc = [&](float x)
				{
					auto w = .5f - .5f * std::cos(TauF * x);
					return w;
				};
				for (auto i = 0; i < FFTSize; ++i)
				{
					const auto x = static_cast<float>(i) * FFTSizeInv;
					const auto wndw = windowFunc(x);
					fifo[i] = buffer[i] * wndw;
				}
				fft.performRealOnlyForwardTransform(fifo, true);
				generateMagnitudes(fifo);
				normalize(fifo);
			}

			void generateMagnitudes(float* bins) noexcept
			{
				for (auto s = 0; s < FFTSize; ++s)
					bins[s] = std::abs(bins[s]);
			}

			void normalize(float* bins) noexcept
			{
				auto max = 0.f;
				for (auto s = 0; s < FFTSize; ++s)
					if (max < bins[s])
						max = bins[s];
				const auto gain = 1.f / max;
				for (auto s = 0; s < FFTSize; ++s)
					bins[s] *= gain;
			}

			void applyLowpassFIR(float* dest, const float* src,
				float cutoff, float bw)
			{
				ImpulseResponse<float, FFTSize> ir;
				ir.makeLowpass(44100.f, cutoff, bw, false);
				const auto lpLen = FFTSize + ir.getLatency();

				std::vector<int> wHead;
				wHead.reserve(lpLen);
				for (auto i = 0; i < lpLen; ++i)
					wHead.emplace_back(i);

				dsp::Convolver<float, FFTSize> convolver(ir);
				for (auto i = 0; i < lpLen; ++i)
					dest[i] = convolver.processSample(src[i], convolver.ringBuffer[0].data(), wHead[i]);
				applyNegativeDelay(dest, ir.getLatency());
			}

			void applyLowpassIIR(float* dest, const float* src,
				float cutoff, float reso, int slope = 1)
			{
				moog::BiquadFilter lp;
				lp.setType(moog::BiquadFilter::Type::LP);
				lp.setCutoffFc(cutoff / 44100.f);
				lp.setResonance(reso);
				lp.update();

				for(auto i = FFTSize - 1; i != 0; --i)
					dest[i] = static_cast<float>(lp(static_cast<double>(src[i])));
				for(auto i = 0; i < FFTSize; ++i)
					dest[i] = static_cast<float>(lp(static_cast<double>(dest[i])));

				for (auto i = 1; i < slope; ++i)
				{
					for (auto j = FFTSize - 1; j != 0; --j)
						dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
					for (auto j = 0; j < FFTSize; ++j)
						dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
				}
			}

			void applyNegativeDelay(float* bins, int offset) noexcept
			{
				for (auto s = 0; s < FFTSize; ++s)
					bins[s] = bins[(s + offset) % FFTSize];
			}

			void divide(float* dest, const float* a, const float* b) noexcept
			{
				for (auto i = 0; i < FFTSize; ++i)
					if (b[i] != 0.f)
						dest[i] = a[i] / b[i];
					else
						dest[i] = 0.f;
			}

			int getMaxMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) const noexcept
			{
				auto idx = lowerLimitIdx;
				auto max = bins[lowerLimitIdx];
				for (auto b = lowerLimitIdx; b < upperLimitIdx; ++b)
					if (max < bins[b])
					{
						max = bins[b];
						idx = b;
					}
				return idx;
			}

			int getMinMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) const noexcept
			{
				auto idx = lowerLimitIdx;
				auto min = bins[lowerLimitIdx];
				for (auto b = lowerLimitIdx; b < upperLimitIdx; ++b)
					if (min > bins[b])
					{
						min = bins[b];
						idx = b;
					}
				return idx;
			}

			float generatePeakGroups(std::vector<PeakIndexInfo>& peakGroups, const float* binsNorm, int minBinIdx, int numFilters)
			{
				auto targetThresholdDb = 0.f;
				do
				{
					targetThresholdDb -= 2.f;
					applyAdaptiveThreshold(peakGroups, binsNorm, targetThresholdDb, minBinIdx);
				} while (peakGroups.size() < numFilters);

				return targetThresholdDb;
			}

			void applyAdaptiveThreshold(std::vector<PeakIndexInfo>& peakGroups, const float* binsNorm,
				float targetThresholdDb, int minBinIdx)
			{
				const auto targetThreshold = math::dbToAmp(targetThresholdDb);
				peakGroups.clear();

				for (auto i = minBinIdx; i < FFTSize; ++i)
					if (binsNorm[i] > targetThreshold)
					{
						auto& peakInfo = peakGroups.emplace_back();
						peakInfo.indexes.push_back(i);

						const auto numNeighbors = 12;
						auto j = i;
						bool stillPeaking = false;
						do
						{
							stillPeaking = false;

							for (auto n = 0; n < numNeighbors; ++n)
							{
								++j;
								if (j >= FFTSize)
									return;
								if (binsNorm[j] > targetThreshold)
								{
									peakInfo.indexes.push_back(j);
									stillPeaking = true;
									n = numNeighbors;
								}
							}
						} while (stillPeaking);

						i = j + 1;
					}
			}

			void generatePeakIndexes(std::vector<int>& peakIndexes, const float* bins,
				const std::vector<PeakIndexInfo>& peakIdxInfos)
			{
				for (auto i = 0; i < peakIndexes.size(); ++i)
					peakIndexes[i] = getMaxMagnitudeIdx(bins, peakIdxInfos[i].indexes.front(), peakIdxInfos[i].indexes.back() + 1);
			}

			float getHarm0Idx(const float* fifo, const int* peakIndexes) const noexcept
			{
				auto peakIdx = peakIndexes[0];
				auto peakMag = fifo[peakIdx];
				auto harm0Mag = peakMag;
				auto harm0Idx = static_cast<float>(peakIdx);
				for (auto i = 1; i < NumFilters; ++i)
				{
					peakIdx = peakIndexes[i];
					peakMag = fifo[peakIdx];
					if (harm0Mag < peakMag)
					{
						harm0Mag = peakMag;
						harm0Idx = static_cast<float>(peakIdx);
					}
				}
				return harm0Idx;
			}

			int getMinPeakIdx(const float* fifo, const int* peakIndexes, int numFilters) const noexcept
			{
				auto j = 0;
				auto peakIdx = peakIndexes[0];
				auto peakMag = fifo[peakIdx];
				for (auto i = 1; i < numFilters; ++i)
				{
					const auto peakIdx1 = peakIndexes[i];
					const auto peakMag1 = fifo[peakIdx1];
					if (peakMag1 < peakMag)
					{
						peakMag = peakMag1;
						peakIdx = peakIdx1;
						j = i;
					}
				}
				return j;
			}

			void generatePeakInfos(const float* fifo, const int* peakIndexes, float harm0Idx) noexcept
			{
				for (auto i = 0; i < NumFilters; ++i)
				{
					const auto peakIdx = peakIndexes[i];
					peakInfos[i].ratio = static_cast<float>(peakIdx) / harm0Idx;
					peakInfos[i].mag = fifo[peakIdx];
				}
			}

			void drawSpectrum(String&& name, const float* buf, float thresholdDb,
				const int* peakIndexes, bool isLog)
			{
				makeSpectrumImage(std::move(name), buf, FFTSize, thresholdDb - 6.f, false,
					peakIndexes, NumFilters, true, isLog, audioSampleRate);
			}
		};

		ModalFilter(const arch::XenManager& _xen) :
			xen(_xen),
			filters(),
			sampleRate(1.),
			nyquist(.5),
			material(),
			reso(-1.),
			numFiltersBelowNyquist(0)
		{
			/*
			material.audioSampleRate = 44100.f;
			auto phase = 0.f;
			const auto inc = 393.f / 44100.f;
			for(auto s = 0; s < material.buffer.size(); ++s)
			{
				material.buffer[s] = 2.f * phase - 1.f;
				phase += inc;
				if (phase >= 1.f)
					--phase;
			}
			material.load();
			*/

			material.load(BinaryData::Eiskaffee_wav, BinaryData::Eiskaffee_wavSize);

			for(auto& filter: filters)
				filter.setSlope(2);
		}

		void prepare(double _sampleRate)
		{
			sampleRate = _sampleRate;
			nyquist = sampleRate * .5;
			setFrequencyHz(1000.);
		}

		/* samples, midi, reso[0,1], numChannels, numSamples */
		void operator()(double** samples, const MidiBuffer& midi,
			double _reso, int numChannels, int numSamples) noexcept
		{
			if (reso != _reso)
			{
				reso = _reso;
				for (auto& filter : filters)
				{
					filter.setResonance(reso);
					filter.update();
				}
			}
			
			auto s = 0;
			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					const auto ts = it.samplePosition;
					const auto xenScale = xen.getXen();
					const auto pitch = static_cast<double>(msg.getNoteNumber()) + xenScale;
					const auto freq = xen.noteToFreqHzWithWrap(pitch);
					setFrequencyHz(freq);
					applyFilter(samples, numChannels, s, ts);
					s = ts;
				}
			}

			applyFilter(samples, numChannels, s, numSamples);
		}

		Material& getMaterial() noexcept
		{
			return material;
		}

		void setFrequencyHz(double freq) noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto& peakInfo = material.peakInfos[i];
				const auto freqRatio = peakInfo.ratio;
				const auto freqFilter = freq * freqRatio;
				if (freqFilter < nyquist)
				{
					const auto fc = math::freqHzToFc(freqFilter, sampleRate);
					auto& filter = filters[i];
					filter.setCutoffFc(fc);
					filter.update();
				}
				else
				{
					numFiltersBelowNyquist = i;
					return;
				}
			}
			numFiltersBelowNyquist = NumFilters;
		}

	protected:
		const arch::XenManager& xen;
		std::array<StereoFilter, NumFilters> filters;
		double sampleRate, nyquist;
		Material material;
		double reso;
		int numFiltersBelowNyquist;

		void applyFilter(double** samples, int numChannels, int startIdx, int endIdx) noexcept
		{
			const auto& peakInfos = material.peakInfos;

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				for (auto i = startIdx; i < endIdx; ++i)
				{
					const auto dry = smpls[i];
					auto wet = filters[0](dry, ch) * peakInfos[0].mag;
					for (auto f = 1; f < numFiltersBelowNyquist; ++f)
						wet += filters[f](dry, ch) * peakInfos[f].mag;
					smpls[i] = wet;
				}
			}
		}
	};
}

/*

todo:
saturate the mags with a parameter (lookuptable)

*/