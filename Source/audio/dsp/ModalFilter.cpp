#include "ModalFilter.h"

namespace dsp
{
	namespace modal
	{
		Material::Material() :
			buffer(),
			peakInfos(),
			updated(false),
			sampleRate(0.f),
			numFilters(NumFilters)
		{
		}

		void Material::load(const char* data, int size)
		{
			fillBuffer(data, size);
			load();
		}

		void Material::load()
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
			generatePeakGroups(peakGroups, binsNorm.data(), harm0Idx);
			std::vector<int> peakIndexes;
			peakIndexes.resize(numFilters);
			generatePeakIndexes(peakIndexes, bins, peakGroups);
			generatePeakInfos(bins, peakIndexes.data(), static_cast<float>(harm0Idx));
			updated.store(true);
		}

		void Material::fillBuffer(const char* data, int size)
		{
			AudioBufferF tmpBuffer;
			sampleRate = static_cast<float>(loadFromMemory(tmpBuffer, data, size));
			const auto samples = tmpBuffer.getArrayOfReadPointers();
			const auto numChannels = tmpBuffer.getNumChannels();
			const auto numSamples = tmpBuffer.getNumSamples();
			for (auto& b : buffer)
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

		void Material::applyFFT(float* fifo)
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

		void Material::generateMagnitudes(float* bins) noexcept
		{
			for (auto s = 0; s < FFTSize; ++s)
				bins[s] = std::abs(bins[s]);
		}

		void Material::normalize(float* bins) noexcept
		{
			auto max = 0.f;
			for (auto s = 0; s < FFTSize; ++s)
				if (max < bins[s])
					max = bins[s];
			const auto gain = 1.f / max;
			for (auto s = 0; s < FFTSize; ++s)
				bins[s] *= gain;
		}

		void Material::applyLowpassFIR(float* dest, const float* src,
			float cutoff, float bw)
		{
			auto irPtr = new ImpulseResponse<float, FFTSize>();
			auto& ir = *irPtr;
			ir.makeLowpass(44100.f, cutoff, bw, false);
			const auto lpLen = FFTSize + ir.getLatency();

			std::vector<int> wHead;
			wHead.reserve(lpLen);
			for (auto i = 0; i < lpLen; ++i)
				wHead.emplace_back(i);

			auto convolverPtr = new dsp::Convolver<float, FFTSize>(ir);
			auto& convolver = *convolverPtr;
			for (auto i = 0; i < lpLen; ++i)
				dest[i] = convolver.processSample(src[i], convolver.ringBuffer[0].data(), wHead[i]);
			applyNegativeDelay(dest, ir.getLatency());

			delete irPtr;
			delete convolverPtr;
		}

		void Material::applyLowpassIIR(float* dest, const float* src,
			float cutoff, float reso, int slope)
		{
			moog::BiquadFilter lp;
			lp.setType(moog::BiquadFilter::Type::LP);
			lp.setCutoffFc(cutoff / 44100.f);
			lp.setResonance(reso);
			lp.update();

			for (auto i = FFTSize - 1; i != 0; --i)
				dest[i] = static_cast<float>(lp(static_cast<double>(src[i])));
			for (auto i = 0; i < FFTSize; ++i)
				dest[i] = static_cast<float>(lp(static_cast<double>(dest[i])));

			for (auto i = 1; i < slope; ++i)
			{
				for (auto j = FFTSize - 1; j != 0; --j)
					dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
				for (auto j = 0; j < FFTSize; ++j)
					dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
			}
		}

		void Material::applyNegativeDelay(float* bins, int offset) noexcept
		{
			for (auto s = 0; s < FFTSize; ++s)
				bins[s] = bins[(s + offset) % FFTSize];
		}

		void Material::divide(float* dest, const float* a, const float* b) noexcept
		{
			for (auto i = 0; i < FFTSize; ++i)
				if (b[i] != 0.f)
					dest[i] = a[i] / b[i];
				else
					dest[i] = 0.f;
		}

		int Material::getMaxMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) const noexcept
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

		int Material::getMinMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) const noexcept
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

		float Material::generatePeakGroups(std::vector<PeakIndexInfo>& peakGroups,
			const float* binsNorm, int minBinIdx)
		{
			auto targetThresholdDb = 0.f;
			do
			{
				targetThresholdDb -= 2.f;
				applyAdaptiveThreshold(peakGroups, binsNorm, targetThresholdDb, minBinIdx);
			} while (peakGroups.size() < numFilters && targetThresholdDb > -240.f);

			if (peakGroups.size() < numFilters)
				numFilters = static_cast<int>(peakGroups.size());

			return targetThresholdDb;
		}

		void Material::applyAdaptiveThreshold(std::vector<PeakIndexInfo>& peakGroups, const float* binsNorm,
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

		void Material::generatePeakIndexes(std::vector<int>& peakIndexes, const float* bins,
			const std::vector<PeakIndexInfo>& peakIdxInfos)
		{
			for (auto i = 0; i < numFilters; ++i)
				peakIndexes[i] = getMaxMagnitudeIdx(bins, peakIdxInfos[i].indexes.front(), peakIdxInfos[i].indexes.back() + 1);
		}

		float Material::getHarm0Idx(const float* fifo, const int* peakIndexes) const noexcept
		{
			auto peakIdx = peakIndexes[0];
			auto peakMag = fifo[peakIdx];
			auto harm0Mag = peakMag;
			auto harm0Idx = static_cast<float>(peakIdx);
			for (auto i = 1; i < numFilters; ++i)
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

		int Material::getMinPeakIdx(const float* fifo, const int* peakIndexes) const noexcept
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

		void Material::generatePeakInfos(const float* bins, const int* peakIndexes, float harm0Idx) noexcept
		{
			for (auto i = 0; i < numFilters; ++i)
			{
				const auto peakIdx = peakIndexes[i];
				peakInfos[i].ratio = static_cast<float>(peakIdx) / harm0Idx;
				peakInfos[i].mag = bins[peakIdx];
			}
		}

		void Material::drawSpectrum(String&& name, const float* buf, float thresholdDb,
			const int* peakIndexes, bool isLog)
		{
			makeSpectrumImage(std::move(name), buf, FFTSize, thresholdDb - 6.f, false,
				peakIndexes, numFilters, true, isLog, sampleRate);
		}
	}

}