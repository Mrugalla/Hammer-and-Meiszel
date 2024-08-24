#pragma once
#include "../../Using.h"
#include <juce_dsp/juce_dsp.h>
#include "Axiom.h"

namespace dsp
{
	namespace modal2
	{
		struct PeakInfo
		{
			double mag, ratio, freqHz;
		};

		struct MaterialData
		{
			MaterialData();

			PeakInfo& operator[](int) noexcept;

			const PeakInfo& operator[](int) const noexcept;

			double getMag(int) const noexcept;

			double getRatio(int) const noexcept;

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

			// data, size
			void load(const char*, int);

			void load();

			void updatePeakInfosFromGUI() noexcept;

			void reportEndGesture() noexcept;

			void reportUpdate() noexcept;

			// sampleRate, samples, numChannels, numSamples
			void fillBuffer(float, const float* const*, int, int);

			MaterialBuffer buffer;
			MaterialData peakInfos;
			std::atomic<Status> status;
			String name;
			float sampleRate;
			std::atomic<bool> soloing;
		private:
			struct PeakIndexInfo
			{
				std::vector<int> indexes;
			};

			void sortPeaks() noexcept;

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

		using ActivesArray = std::array<bool, NumFilters>;

		struct DualMaterial
		{
			DualMaterial() :
				materials(),
				actives()
			{
				generateSaw(materials[0]);
				generateSquare(materials[1]);
				for (auto& active : actives)
					active = true;
			}

			const bool updated() const noexcept
			{
				bool u = false;
				for (auto m = 0; m < 2; ++m)
				{
					const auto& material = materials[m];
					const auto& status = material.status;
					if (status == Status::UpdatedMaterial)
						u = true;
				}
				return u;
			}

			void reportUpdate() noexcept
			{
				for (auto m = 0; m < 2; ++m)
				{
					auto& material = materials[m];
					auto& status = material.status;
					status = Status::UpdatedProcessor;
				}
			}

			Material& getMaterial(int i) noexcept
			{
				return materials[i];
			}

			const Material& getMaterial(int i) const noexcept
			{
				return materials[i];
			}

			const MaterialData& getMaterialData(int i) const noexcept
			{
				return materials[i].peakInfos;
			}

			const bool isActive(int i) const noexcept
			{
				return actives[i];
			}

			ActivesArray& getActives() noexcept
			{
				return actives;
			}

		protected:
			std::array<Material, 2> materials;
			ActivesArray actives;
		};
	}
}