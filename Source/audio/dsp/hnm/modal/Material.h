#pragma once
#include "../../../Using.h"
#include "../../../../arch/State.h"
#include <juce_dsp/juce_dsp.h>
#include "Axiom.h"

namespace dsp
{
	namespace modal
	{
		struct PeakInfo
		{
			double mag, ratio, freqHz, keytrack;
		};

		struct MaterialData
		{
			MaterialData();

			PeakInfo& operator[](int) noexcept;

			const PeakInfo& operator[](int) const noexcept;

			double getMag(int) const noexcept;

			double getRatio(int) const noexcept;

			double getFreqHz(int) const noexcept;

			double getKeytrack(int) const noexcept;

			void copy(const MaterialData&) noexcept;
		private:
			std::array<PeakInfo, NumFilters> data;
		};

		struct MaterialDataStereo
		{
			MaterialDataStereo();

			// ch
			MaterialData& operator[](int) noexcept;

			// ch
			const MaterialData& operator[](int) const noexcept;

			// ch,i
			PeakInfo& operator()(int, int) noexcept;

			// ch,i
			const PeakInfo& operator()(int, int) const noexcept;

			// ch,i
			double getMag(int, int) const noexcept;

			// ch,i
			double getRatio(int, int) const noexcept;

			// other, numChannels
			void copy(const MaterialDataStereo&, int) noexcept;
		private:
			std::array<MaterialData, 2> data;
		};

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

			Material();

			void savePatch(arch::State& state, String&& matStr) const
			{
				for (auto j = 0; j < NumFilters; ++j)
				{
					const auto& peakInfo = peakInfos[j];
					const auto peakStr = matStr + "pk" + juce::String(j);
					state.set(peakStr + "mg", peakInfo.mag);
					state.set(peakStr + "rt", peakInfo.ratio);
					state.set(peakStr + "fr", peakInfo.freqHz);
				}
			}

			void loadPatch(const arch::State& state, String&& matStr)
			{
				for (auto j = 0; j < NumFilters; ++j)
				{
					auto& peakInfo = peakInfos[j];
					const auto peakStr = matStr + "pk" + juce::String(j);
					const auto magVal = state.get(peakStr + "mg");
					if (magVal != nullptr)
						peakInfo.mag = static_cast<double>(*magVal);
					const auto ratioVal = state.get(peakStr + "rt");
					if (ratioVal != nullptr)
						peakInfo.ratio = static_cast<double>(*ratioVal);
					const auto freqVal = state.get(peakStr + "fr");
					if (freqVal != nullptr)
						peakInfo.freqHz = static_cast<double>(*freqVal);
				}
				updatePeakInfosFromGUI();
			}

			// data, size
			void load(const char*, int);

			void load();

			void updatePeakInfosFromGUI() noexcept;

			void reportEndGesture() noexcept;

			void reportUpdate() noexcept;

			void updateKeytrackValues() noexcept;

			// sampleRate, samples, numChannels, numSamples
			void fillBuffer(float, const float* const*, int, int);

			void sortPeaks() noexcept;

			MaterialBuffer buffer;
			MaterialData peakInfos;
			std::atomic<StatusMat> status;
			String name;
			float sampleRate;
			std::atomic<bool> soloing;
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

			// dest, a, b, mixA
			void divide(float*, const float*, const float*, float) noexcept;

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
			void drawSpectrum(const String&, const float*, float,
				const int*, bool);
		};

		void generateSine(Material&);
		void generateSaw(Material&);
		void generateSquare(Material&);
		void generateFibonacci(Material&);
		void generatePrime(Material&);

		using ActivesArray = std::array<bool, NumFilters>;

		struct DualMaterial
		{
			DualMaterial();

			const bool updated() const noexcept;

			void reportUpdate() noexcept;

			Material& getMaterial(int) noexcept;

			const Material& getMaterial(int) const noexcept;

			const MaterialData& getMaterialData(int) const noexcept;

			const bool isActive(int) const noexcept;

			ActivesArray& getActives() noexcept;

		protected:
			std::array<Material, 2> materials;
			ActivesArray actives;
		};
	}
}