#pragma once
#include "Voice.h"
#include "../AutoGain.h"

namespace dsp
{
	namespace modal
	{
		struct Filtr
		{
			Filtr(const arch::XenManager&);

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