#pragma once
#include "../../arch/XenManager.h"
#include "../../libs/MoogLadders-master/src/Filters.h"
#include "../../arch/Math.h"
#include "PRM.h"
#include "Convolver.h"
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
		static constexpr int NumFilters = 12;
#endif

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

			void updatePeakInfosFromGUI() noexcept
			{
				auto magMax = peakInfos[0].mag;
				for (int i = 1; i < NumFilters; ++i)
					magMax = std::max(magMax, peakInfos[i].mag);
				if (magMax != 1.f)
				{
					auto g = 1.f / magMax;
					for(auto &p : peakInfos)
						p.mag *= g;
				}
				updateState.store(UpdateState::UpdateProcessor);
			}

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

		using ResonatorArray = std::array<ResonatorStereo2, NumFilters>;

		struct Resonator
		{
			struct Val
			{
				Val();

				double getFreq(const arch::XenManager&) noexcept;

				double pitch, transpose, pb, pbRange, xen;
			};

			Resonator(const arch::XenManager&, const DualMaterial&);

			void reset() noexcept;

			void prepare(double);

			// samples, midi, _transposeSemi, numChannels, numSamples
			void operator()(double**, const MidiBuffer&, double, int, int) noexcept;

			// freqHz
			void setFrequencyHz(double) noexcept;

			void updateFreqRatios() noexcept;

			// bw
			void setBandwidth(double) noexcept;

		private:
			const arch::XenManager& xen;
			const DualMaterial& material;
			ResonatorArray resonators;
			Val val;
			double freqHz, sampleRate, nyquist;
			int numFiltersBelowNyquist;

			/* samples, midi, numChannels, numSamples */
			void process(double**, const MidiBuffer&, int, int) noexcept;

			// samples, numChannels, startIdx, endIdx
			void applyFilter(double**, int, int, int) noexcept;
		};

		struct Voice
		{
			Voice(const arch::XenManager&, const DualMaterial&);

			void prepare(double);

			// bw
			void setBandwidth(double) noexcept;

			// samplesSrc, samplesDest, midi, transposeSemi, numChannels, numSamples
			void operator()(const double**, double**, const MidiBuffer&, double, int, int) noexcept;

			// sleepy, samplesDest, numChannels, numSamples
			void detectSleepy(bool&, double**, int, int) noexcept;

			Resonator filter;
			Envelope env;
			std::array<double, BlockSize2x> envBuffer;
			double gain;
		protected:

			// midi, numSamples
			void synthesizeEnvelope(const MidiBuffer&, int) noexcept;

			// s, ts
			int synthesizeEnvelope(int, int) noexcept;

			// samplesSrc, samplesDest, numChannels, numSamples
			void processEnvelope(const double**, double**, int, int) noexcept;

			// samplesDest, numChannels, numSamples
			bool bufferSilent(double* const*, int, int) const noexcept;
		};

		struct Filter
		{
			Filter(const arch::XenManager&);

			void prepare(double);

			// modalMix[0,1], modalHarmonize[0,1], modalSaturate[-1,1], reso[0,1]
			void updateParameters(double, double, double, double) noexcept;

			// samplesSrc, samplesDest, midi, transposeSemi, numChannels, numSamples, voiceIdx
			void operator()(const double**, double**, const MidiBuffer&,
				double, int, int, int) noexcept;

			AutoGain5 autoGainReso;
			DualMaterial material;
			std::array<Voice, NumMPEChannels> voices;
			double modalMix, modalHarmonize, modalSaturate, reso, sampleRateInv;
		private:
			// modalMix, modalHarmonize, modalSaturate
			void updateModalMix(double, double, double) noexcept;

			void updateReso(double) noexcept;

			void initAutoGainReso();
		};
	}
	
}

/*

todo:

performance:
	filter performance
		SIMD
		try copying filter state without recalcing coefficients
			reso
			freq

features:
	-

*/