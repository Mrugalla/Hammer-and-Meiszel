#pragma once
#include "../../arch/XenManager.h"
//#include "../../libs/MoogLadders-master/src/Filters.h"
#include "../../arch/Math.h"
//#include "Convolver.h"
#include <BinaryData.h>
//#include <juce_dsp/juce_dsp.h>
#include "modal/Material.h"
#include "Resonator.h"
#include "AutoGain.h"
#include "EnvelopeGenerator.h"

namespace dsp
{
	namespace modal
	{
#if JUCE_DEBUG
		static constexpr int NumFilters = 12;
#else
		static constexpr int NumFilters = 12;
#endif

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

			// atk, dcy, sus, rls
			void updateParameters(double, double, double, double) noexcept;

			// bwFc
			void setBandwidth(double) noexcept;

			// samplesSrc, samplesDest, midi, transposeSemi, numChannels, numSamples
			void operator()(const double**, double**, const MidiBuffer&, double, int, int) noexcept;

			// sleepy, samplesDest, numChannels, numSamples
			void detectSleepy(bool&, double**, int, int) noexcept;

			Resonator filter;
			EnvelopeGenerator env;
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