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
		static constexpr int NumFilters = 24;
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
				double mag, ratio;
			};

			Material();

			// data, size
			void load(const char*, int);

			void load();

			MaterialBuffer buffer;
			std::array<PeakInfo, NumFilters> peakInfos;
			std::atomic<bool> updated;
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

			// mix, harmonize
			void setMix(double, double) noexcept;

			// idx
			double getMag(int) const noexcept;

			// idx
			double getRatio(int) const noexcept;

		protected:
			std::array<Material, 2> materials;
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
			Resonator(const arch::XenManager&, const DualMaterial&);

			void prepare(double);

			// samples, midi, numChannels, numSamples
			void operator()(double**, const MidiBuffer&, int, int) noexcept;

			void setFrequencyHz(double) noexcept;

			void updateFreqRatios() noexcept;

			// bw
			void setBandwidth(double) noexcept;

		private:
			const arch::XenManager& xen;
			const DualMaterial& material;
			ResonatorArray resonators;
			double freqHz, sampleRate, nyquist;
			int numFiltersBelowNyquist;

			// samples, numChannels, startIdx, endIdx
			void applyFilter(double**, int, int, int) noexcept;
		};

		struct Voice
		{
			Voice(const arch::XenManager&, const DualMaterial&);

			void prepare(double);

			// bw
			void setBandwidth(double) noexcept;

			// samplesSrc, samplesDest, midi, numChannels, numSamples
			void operator()(const double**, double**, const MidiBuffer&, int, int) noexcept;

			Resonator filter;
			Envelope env;
			std::array<double, BlockSize2x> envBuffer;
			double gain;
			bool sleepy;
		protected:
			// samplesSrc, samplesDest, midi, numChannels, numSamples
			void processBlock(const double**, double**, const MidiBuffer&, int, int) noexcept;

			// midi, numSamples
			void synthesizeEnvelope(const MidiBuffer&, int) noexcept;

			// s, ts
			int synthesizeEnvelope(int, int) noexcept;

			// samplesSrc, samplesDest, numChannels, numSamples
			void processEnvelope(const double**, double**, int, int) noexcept;

			// samplesDest, numChannels, numSamples
			void detectSleepy(double**, int, int) noexcept;

			// samplesDest, numChannels, numSamples
			bool bufferSilent(double* const*, int, int) const noexcept;
		};

		struct Filter
		{
			Filter(const arch::XenManager&);

			void prepare(double);

			// samplesSrc, voiceSplit, parallelProcessor, modalMix[0,1], modalHarmonize[0,1], reso[0,1], numChannels, numSamples
			void operator()(const double**, const MPESplit&, PPMIDIBand&,
				double, double, double, int, int) noexcept;

			AutoGain5 autoGainReso;
			DualMaterial material;
			std::array<Voice, NumMPEChannels> voices;
			double modalMix, modalHarmonize, reso, sampleRateInv;
		private:
			// modalMix, modalHarmonize
			void updateModalMix(double, double) noexcept;

			void updateReso(double) noexcept;

			// samplesSrc, parallelProcessor, voiceSplit, v, numChannels, numSamples
			void processVoice(const double**, PPMIDIBand&, const MPESplit&,
				int, int, int) noexcept;

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
	material
		mags
			saturate
		freqRatios
			harmonize (nearest integer)

*/