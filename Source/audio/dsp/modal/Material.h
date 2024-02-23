#pragma once
#include "../../Using.h"
#include <juce_dsp/juce_dsp.h>

namespace dsp
{
	namespace modal
	{
		struct Material
		{
			enum class UpdateState
			{
				Idle,
				UpdateProcessor,
				UpdateGUI
			};

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
				double mag, ratio;
			};

			Material();

			// data, size
			void load(const char*, int);

			void load();

			void updatePeakInfosFromGUI() noexcept;

			MaterialBuffer buffer;
			std::array<PeakInfo, NumFilters> peakInfos;
			std::atomic<UpdateState> updateState;
			float sampleRate;
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

		struct DualMaterial
		{
			DualMaterial();

			// mix, harmonize, saturate
			void setMix(double, double, double) noexcept;

			// idx
			double getMag(int) const noexcept;

			// idx
			double getRatio(int) const noexcept;

			bool wantsUpdate() const noexcept;

			std::array<Material, 2> materials;
		protected:
			std::array<Material::PeakInfo, NumFilters> peakInfos;
		};
	}
}