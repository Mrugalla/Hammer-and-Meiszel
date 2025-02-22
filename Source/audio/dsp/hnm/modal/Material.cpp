#include "Material.h"
#include "../../Convolver.h"
#include "../../../../libs/MoogLadders-master/src/Filters.h"
#include <BinaryData.h>

namespace dsp
{
	namespace modal
	{
		// MATERIALDATA

		MaterialData::MaterialData() :
			partials()
		{}

		Partial& MaterialData::operator[](int i) noexcept
		{
			return partials[i];
		}

		const Partial& MaterialData::operator[](int i) const noexcept
		{
			return partials[i];
		}

		double MaterialData::getMag(int i) const noexcept
		{
			return partials[i].mag;
		}

		double MaterialData::getFc(int i) const noexcept
		{
			return partials[i].fc;
		}

		void MaterialData::copy(const MaterialData& m) noexcept
		{
			for (auto i = 0; i < NumPartials; ++i)
			{
				partials[i].mag = m.partials[i].mag;
				partials[i].fc = m.partials[i].fc;
			}
		}

		MaterialData::Array& MaterialData::data() noexcept
		{
			return partials;
		}

		// MATERIALDATASTEREO

		MaterialDataStereo::MaterialDataStereo() :
			data()
		{}

		MaterialData& MaterialDataStereo::operator[](int ch) noexcept
		{
			return data[ch];
		}

		const MaterialData& MaterialDataStereo::operator[](int ch) const noexcept
		{
			return data[ch];
		}

		Partial& MaterialDataStereo::operator()(int ch, int i) noexcept
		{
			return data[ch][i];
		}

		const Partial& MaterialDataStereo::operator()(int ch, int i) const noexcept
		{
			return data[ch][i];
		}

		double MaterialDataStereo::getMag(int ch, int i) const noexcept
		{
			return data[ch][i].mag;
		}

		double MaterialDataStereo::getFc(int ch, int i) const noexcept
		{
			return data[ch][i].fc;
		}

		void MaterialDataStereo::copy(const MaterialDataStereo& m, int numChannels) noexcept
		{
			data[0].copy(m.data[0]);
			if (numChannels == 2)
				data[1].copy(m.data[1]);
		}
		
		// MATERIAL (FREE FUNCTIONS OUTTAKES)

		void divide(float* dest, const float* a, const float* b, float mixA) noexcept
		{
			for (auto i = 0; i < Material::FFTSize; ++i)
				if (b[i] != 0.f)
				{
					const auto ab = a[i] / b[i];
					dest[i] = ab + mixA * (a[i] - ab);
				}
				else
					dest[i] = mixA * a[i];
		}

		void applyNegativeDelay(float* bins, int offset) noexcept
		{
			for (auto s = 0; s < Material::FFTSize; ++s)
				bins[s] = bins[(s + offset) % Material::FFTSize];
		}

		void applyLowpassFIR(float* dest, const float* src,
			float cutoff, float bw)
		{
			auto irPtr = new ImpulseResponse<float, Material::FFTSize>();
			auto& ir = *irPtr;
			ir.makeLowpass(44100.f, cutoff, bw, false);
			const auto lpLen = Material::FFTSize + ir.getLatency();

			std::vector<int> wHead;
			wHead.reserve(lpLen);
			for (auto i = 0; i < lpLen; ++i)
				wHead.emplace_back(i);

			auto convolverPtr = new dsp::Convolver<float, Material::FFTSize>(ir);
			auto& convolver = *convolverPtr;
			for (auto i = 0; i < lpLen; ++i)
				dest[i] = convolver.processSample(src[i], convolver.ringBuffer[0].data(), wHead[i]);
			applyNegativeDelay(dest, ir.getLatency());

			delete irPtr;
			delete convolverPtr;
		}

		void applyLowpassIIR(float* dest, const float* src,
			float cutoff, float reso, int slope)
		{
			moog::BiquadFilter lp;
			lp.setType(moog::BiquadFilter::Type::LP);
			lp.setCutoffFc(cutoff / 44100.f);
			lp.setResonance(reso);
			lp.update();

			for (auto i = Material::FFTSize - 1; i != 0; --i)
				dest[i] = static_cast<float>(lp(static_cast<double>(src[i])));
			for (auto i = 0; i < Material::FFTSize; ++i)
				dest[i] = static_cast<float>(lp(static_cast<double>(dest[i])));

			for (auto i = 1; i < slope; ++i)
			{
				for (auto j = Material::FFTSize - 1; j != 0; --j)
					dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
				for (auto j = 0; j < Material::FFTSize; ++j)
					dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
			}
		}

		void updateKeytrackValues(MaterialData& peakInfos) noexcept
		{
			for (auto i = 0; i < NumPartials; ++i)
			{
				const auto fc = peakInfos[i].fc;
				auto keytrack = (1. + std::cos(Tau * fc)) * .5;
				const auto ratioFrac = fc - std::floor(fc);
				if (ratioFrac < .03 || ratioFrac > .97)
					keytrack = std::round(keytrack);
				//peakInfos[i].keytrack = keytrack;
			}
		}

		int getMinMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) noexcept
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

		float getHarm0Idx(const float* fifo, const int* peakIndexes) noexcept
		{
			auto peakIdx = peakIndexes[0];
			auto peakMag = fifo[peakIdx];
			auto harm0Mag = peakMag;
			auto harm0Idx = static_cast<float>(peakIdx);
			for (auto i = 1; i < NumPartials; ++i)
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

		int getMinPeakIdx(const float* fifo, const int* peakIndexes) noexcept
		{
			auto j = 0;
			auto peakIdx = peakIndexes[0];
			auto peakMag = fifo[peakIdx];
			for (auto i = 1; i < NumPartials; ++i)
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

		void drawSpectrum(const String& text, const float* buf, float thresholdDb,
			const int* peakIndexes, float sampleRate, bool isLog)
		{
			auto numFilters = 0;
			if (peakIndexes != nullptr)
				for (auto i = 0; i < NumPartials; ++i)
					numFilters += peakIndexes[i] != -1 ? 1 : 0;

			makeSpectrumImage(text, buf, Material::FFTSize, thresholdDb - 6.f, false,
				peakIndexes, numFilters, true, isLog, sampleRate);
		}

		void sortByKeytrackDescending(MaterialData& peakInfos) noexcept
		{
			auto& partials = peakInfos.data();
			std::sort(partials.begin(), partials.end(), [](const Partial& a, const Partial& b)
				{
					const auto aFc = a.fc;
					const auto aKeytrack = (1. + std::cos(Tau * aFc)) * .5;

					const auto bFc = b.fc;
					const auto bKeytrack = (1. + std::cos(Tau * bFc)) * .5;

					return aKeytrack > bKeytrack;
				});
		}

		// MATERIAL (FREE FUNCTIONS)

		void applyWindow(float* fifo, const Material::MaterialBuffer& buffer) noexcept
		{
			const auto windowFunc = [&](float x)
			{
				auto w = .5f - .5f * std::cos(TauF * x);
				return math::tanhApprox(2.f * w);
			};

			const auto fftSizeInv = 1.f / static_cast<float>(Material::FFTSize);

			for (auto i = 0; i < Material::FFTSize; ++i)
			{
				const auto x = static_cast<float>(i) * fftSizeInv;
				const auto wndw = windowFunc(x);
				const auto smpl = buffer[i];
				fifo[i] = smpl * wndw;
			}
		}

		void removeDCOffset(float* fifo) noexcept
		{
			for (auto i = 0; i < 3; ++i)
			{
				moog::BiquadFilter hp;
				hp.setType(moog::BiquadFilter::Type::HP);
				hp.setCutoffFc(20. / 44100.);
				hp.setResonance(.2);
				hp.update();
				for (auto j = 0; j < Material::FFTSize; ++j)
					fifo[j] = static_cast<float>(hp(static_cast<double>(fifo[j])));
			}
		}

		void generateMagnitudes(float* bins) noexcept
		{
			for (auto s = 0; s < Material::FFTSize; ++s)
				bins[s] = std::abs(bins[s]);
		}

		void normalize(float* bins) noexcept
		{
			auto max = 0.f;
			for (auto s = 0; s < Material::FFTSize; ++s)
				if (max < bins[s])
					max = bins[s];
			const auto gain = 1.f / max;
			for (auto s = 0; s < Material::FFTSize; ++s)
				bins[s] *= gain;
		}

		void applyFFT(float* fifo, const Material::MaterialBuffer& buffer)
		{
			applyWindow(fifo, buffer);
			removeDCOffset(fifo);
			juce::dsp::FFT fft(Material::FFTOrder);
			fft.performRealOnlyForwardTransform(fifo, true);
			generateMagnitudes(fifo);
			normalize(fifo);
		}

		int getMaxMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) noexcept
		{
			auto idx = lowerLimitIdx;
			auto max = bins[lowerLimitIdx];
			for (auto b = lowerLimitIdx; b < upperLimitIdx; ++b)
			{
				const auto bin = bins[b];
				if (max < bin)
				{
					max = bin;
					idx = b;
				}
			}
			return idx;
		}

		void applyAdaptiveThreshold(std::vector<Material::PeakIndexInfo>& peakGroups, const float* binsNorm,
			float targetThresholdDb, int minBinIdx)
		{
			const auto targetThreshold = math::dbToAmp(targetThresholdDb);
			peakGroups.clear();

			for (auto i = minBinIdx; i < Material::FFTSize; ++i)
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
							if (j >= Material::FFTSize)
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

		float generatePeakGroups(std::vector<Material::PeakIndexInfo>& peakGroups,
			const float* binsNorm, int minBinIdx)
		{
			auto targetThresholdDb = 0.f;
			do
			{
				targetThresholdDb -= 2.f;
				applyAdaptiveThreshold(peakGroups, binsNorm, targetThresholdDb, minBinIdx);
			} while (peakGroups.size() < NumPartials && targetThresholdDb > -60.f);

			if (peakGroups.size() < NumPartials)
			{
				Material::PeakIndexInfo dummy;
				dummy.indexes.push_back(-1);
				peakGroups.resize(NumPartials, dummy);
			}

			return targetThresholdDb;
		}

		void generatePeakIndexes(std::vector<int>& peakIndexes, const float* bins,
			const std::vector<Material::PeakIndexInfo>& peakIdxInfos)
		{
			for (auto i = 0; i < NumPartials; ++i)
			{
				auto& peakIndexInfo = peakIdxInfos[i];
				if (peakIndexInfo.indexes.front() != -1)
					peakIndexes[i] = getMaxMagnitudeIdx(bins, peakIndexInfo.indexes.front(), peakIndexInfo.indexes.back() + 1);
				else
					peakIndexes[i] = -1;
			}

			std::sort(peakIndexes.begin(), peakIndexes.end());
		}

		void generatePeakInfos(MaterialData& peakInfos, const float* bins,
			const int* peakIndexes, float harm0Idx) noexcept
		{
			static constexpr auto FFTSizeInv = 1.f / static_cast<float>(Material::FFTSize);

			for (auto i = 0; i < NumPartials; ++i)
			{
				const auto peakIdx = peakIndexes[i];
				if (peakIdx != -1)
				{
					const auto peakIdxD = static_cast<double>(peakIdx);
					const auto fc = peakIdxD / static_cast<double>(harm0Idx);
					const auto mag = static_cast<double>(bins[peakIdx]);
					peakInfos[i].fc = fc;
					peakInfos[i].mag = mag;
				}
				else
				{
					peakInfos[i].fc = 1.;
					peakInfos[i].mag = 0.;
				}
			}
		}

		void sortRatios(MaterialData& peakInfos) noexcept
		{
			for (int i = 0; i < NumPartials; ++i)
				for (int j = i + 1; j < NumPartials; ++j)
					if (peakInfos[i].fc > peakInfos[j].fc)
						std::swap(peakInfos[i], peakInfos[j]);
		}

		void normalize(MaterialData& peakInfos) noexcept
		{
			auto maxMag = peakInfos[0].mag;
			for (auto i = 1; i < NumPartials; ++i)
				if (maxMag < peakInfos[i].mag)
					maxMag = peakInfos[i].mag;
			if (maxMag == 0. || maxMag == 1.f)
				return;
			const auto gain = 1.f / maxMag;
			for (auto i = 0; i < NumPartials; ++i)
				peakInfos[i].mag *= gain;
		}

		// MATERIAL

		Material::Material() :
			buffer(),
			peakInfos(),
			status(StatusMat::Processing),
			name("init material"),
			sampleRate(0.f),
			soloing(false)
		{
		}

		void Material::savePatch(arch::State& state, const String& matStr) const
		{
			for (auto j = 0; j < NumPartials; ++j)
			{
				const auto& peakInfo = peakInfos[j];
				const auto peakStr = matStr + "pk" + String(j);
				state.set(peakStr + "mg", peakInfo.mag);
				state.set(peakStr + "fc", peakInfo.fc);
			}
		}

		void Material::loadPatch(const arch::State& state, const String& matStr)
		{
			for (auto j = 0; j < NumPartials; ++j)
			{
				auto& peakInfo = peakInfos[j];
				const auto peakStr = matStr + "pk" + juce::String(j);
				const auto magVal = state.get(peakStr + "mg");
				if (magVal != nullptr)
					peakInfo.mag = static_cast<double>(*magVal);
				const auto fcVal = state.get(peakStr + "fc");
				if (fcVal != nullptr)
					peakInfo.fc = static_cast<double>(*fcVal);
			}
			updatePeakInfosFromGUI();
		}

		void Material::load()
		{
			if (math::bufferSilent(buffer.data(), FFTSize))
				return;
			std::vector<float> fifo;
			fifo.resize(FFTSize * 2, 0.f);
			auto bins = fifo.data();
			applyFFT(bins, buffer);
			std::vector<PeakIndexInfo> peakGroups;
			const auto harm0Idx = getMaxMagnitudeIdx(bins, 0, FFTSize / 2);
			generatePeakGroups(peakGroups, bins, harm0Idx);
			std::vector<int> peakIndexes;
			peakIndexes.resize(NumPartials);
			generatePeakIndexes(peakIndexes, bins, peakGroups);
			generatePeakInfos(peakInfos, bins, peakIndexes.data(), static_cast<float>(harm0Idx));
			sortRatios(peakInfos);
			normalize(peakInfos);
			reportUpdate();
		}

		void Material::reportUpdate() noexcept
		{
			status.store(StatusMat::UpdatedMaterial);
		}

		void Material::updatePeakInfosFromGUI() noexcept
		{
			normalize(peakInfos);
			reportUpdate();
		}

		void Material::reportEndGesture() noexcept
		{
			sortRatios(peakInfos);
			updatePeakInfosFromGUI();
		}

		void Material::load(const char* data, int size)
		{
			fillBuffer(data, size);
			load();
		}

		void Material::fillBuffer(const char* data, int size)
		{
			AudioBufferF tmpBuffer;
			const auto _sampleRate = static_cast<float>(loadFromMemory(tmpBuffer, data, size));
			const auto samples = tmpBuffer.getArrayOfReadPointers();
			const auto numChannels = tmpBuffer.getNumChannels();
			const auto numSamples = tmpBuffer.getNumSamples();
			fillBuffer(_sampleRate, samples, numChannels, numSamples);
		}

		void Material::fillBuffer(float _sampleRate, const float* const* samples, int numChannels, int numSamples)
		{
			sampleRate = _sampleRate;
			for (auto& b : buffer)
				b = 0.f;
			auto len = numSamples < FFTSize ? numSamples : FFTSize;
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < len; ++s)
					buffer[s] += samples[ch][s];
			const auto gain = 1.f / static_cast<float>(numChannels);
			SIMD::multiply(buffer.data(), gain, len);
		}

		// GENERATE

		void generateSine(Material& material)
		{
			auto& peaks = material.peakInfos;
			peaks[0].mag = 1.;
			peaks[0].fc = 1.;
			for (auto i = 1; i < NumPartials; ++i)
			{
				auto& peak = peaks[i];
				peak.mag = 0.;
				peak.fc = 1. + static_cast<float>(i);
			}
			material.reportUpdate();
		}

		void generateSaw(Material& material)
		{
			auto& peaks = material.peakInfos;
			const auto numPartilsInv = 1. / static_cast<double>(NumPartials);
			for (auto i = 0; i < NumPartials; ++i)
			{
				auto& peak = peaks[i];
				const auto iF = static_cast<double>(i);
				const auto iR = iF * numPartilsInv;

				peak.mag = 1. - iR;
				peak.fc = 1. + iF;
			}
			material.reportUpdate();
		}

		void generateSquare(Material& material)
		{
			auto& peaks = material.peakInfos;
			const auto numPartilsInv = 1. / static_cast<double>(NumPartials);
			for (auto i = 0; i < NumPartials; ++i)
			{
				auto& peak = peaks[i];
				const auto iF = static_cast<double>(i);
				const auto iR = iF * numPartilsInv;

				peak.mag = 1. - iR;
				peak.fc = 1. + iF * 2.;
			}
			material.reportUpdate();
		}

		void generateFibonacci(Material& material)
		{
			auto& peaks = material.peakInfos;
			for (auto i = 0; i < NumPartials; ++i)
			{
				auto& peak = peaks[i];

				const auto fibonacci = math::fibonacci(i + 2);
				const auto fibD = static_cast<double>(fibonacci);

				peak.fc = fibD;
				peak.mag = 1. / fibD;
			}
			material.reportUpdate();
		}

		void generatePrime(Material& material)
		{
			auto& peaks = material.peakInfos;
			for (auto i = 0; i < NumPartials; ++i)
			{
				auto& peak = peaks[i];

				const auto fibonacci = math::prime(i + 1);
				const auto fibD = static_cast<double>(fibonacci) * .5;

				peak.fc = fibD;
				peak.mag = 1. / fibD;
			}
			material.reportUpdate();
		}

		// DUALMATERIAL

		DualMaterial::DualMaterial() :
			materials(),
			actives()
		{
			generateSaw(materials[0]);
			generateSquare(materials[1]);
			for (auto& active : actives)
				active = true;
		}

		const bool DualMaterial::updated() const noexcept
		{
			bool u = false;
			for (auto m = 0; m < 2; ++m)
			{
				const auto& material = materials[m];
				const auto& status = material.status;
				if (status == StatusMat::UpdatedMaterial)
					u = true;
			}
			return u;
		}

		void DualMaterial::reportUpdate() noexcept
		{
			for (auto m = 0; m < 2; ++m)
			{
				auto& material = materials[m];
				auto& status = material.status;
				status = StatusMat::UpdatedProcessor;
			}
		}

		Material& DualMaterial::getMaterial(int i) noexcept
		{
			return materials[i];
		}

		const Material& DualMaterial::getMaterial(int i) const noexcept
		{
			return materials[i];
		}

		const MaterialData& DualMaterial::getMaterialData(int i) const noexcept
		{
			return materials[i].peakInfos;
		}

		const bool DualMaterial::isActive(int i) const noexcept
		{
			return actives[i];
		}

		ActivesArray& DualMaterial::getActives() noexcept
		{
			return actives;
		}
	}
}