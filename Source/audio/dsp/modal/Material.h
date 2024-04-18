#pragma once
#include "../../Using.h"
#include <juce_dsp/juce_dsp.h>
#include "Axiom.h"

namespace dsp
{
	namespace modal
	{
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
				double mag, ratio;
			};

			using PeakArray = std::array<PeakInfo, NumFilters>;

			Material();

			// data, size
			void load(const char*, int);

			void load();

			void updatePeakInfosFromGUI() noexcept;

			void reportEndGesture() noexcept;

			// sampleRate, samples, numChannels, numSamples
			void fillBuffer(float, const float* const*, int, int);

			MaterialBuffer buffer;
			PeakArray peakInfos;
			std::atomic<Status> status;
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

		using PeakArray = Material::PeakArray;

		void generateSine(Material&);
		void generateSaw(Material&);
		void generateSquare(Material&);

		struct DualMaterial
		{
			DualMaterial();

			// mix, spreizung, harmonize, saturate
			void setMix(double, double, double, double) noexcept;

			// idx
			double getMag(int) const noexcept;

			// idx
			double getRatio(int) const noexcept;

			bool hasUpdated() const noexcept;

			std::array<Material, 2> materials;
			PeakArray peakInfos;
		};
	}
}