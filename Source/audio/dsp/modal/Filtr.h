#pragma once
#include "Voice.h"
#include "../AutoGain.h"
#include "../PRM.h"

namespace dsp
{
	namespace modal
	{
		struct Filtr
		{
			Filtr(const arch::XenManager&);

			void prepare(double);

			// attack, decay, sustain, release,
			// modalMix[0,1], modalSpreizung[-2,2], modalHarmonize[0,1], modalSaturate[-1,1], reso[0,1],
			// transposeSemi[-n,n]
			void updateParameters(double, double, double, double,
				double, double, double, double, double, double) noexcept;

			// samplesSrc, samplesDest, midi, numChannels, numSamples, voiceIdx
			void operator()(const double**, double**, const MidiBuffer&,
				int, int, int) noexcept;

			EnvelopeGenerator::Parameters envGenParams;
			const arch::XenManager& xen;
			AutoGain5 autoGainReso;
			std::array<Material, 2> materials;
			std::array<Voice, NumMPEChannels> voices;
			double sampleRateInv, pbRangeP, xenP, bwFc;
			PRMBlockD blendPRM, spreizungPRM, harmoniePRM, tonalitaetPRM, resoPRM, transposeSemiPRM;
		private:
			// modalMix, modalSpreizung, modalHarmonize, modalSaturate
			void updateModalMix(double, double, double, double) noexcept;

			void updateReso(double) noexcept;

			void updateReso() noexcept;

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