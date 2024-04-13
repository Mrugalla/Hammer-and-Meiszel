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

			// modalMix[0,1], modalSpreizung[-2,2], modalHarmonize[0,1], modalSaturate[-1,1], reso[0,1],
			// transposeSemi[-n,n]
			void updateParameters(double, double, double, double, double, double) noexcept;

			// samplesSrc, samplesDest, midi, numChannels, numSamples, voiceIdx
			void operator()(const double**, double**, const MidiBuffer&,
				int, int, int) noexcept;

			const arch::XenManager& xen;
			AutoGain5 autoGainReso;
			DualMaterial material;
			std::array<Voice, NumMPEChannels> voices;
			double sampleRateInv, pbRangeP, xenP;
			PRMBlockD blendPRM, spreizungPRM, harmoniePRM, tonalitaetPRM, resoPRM, transposeSemiPRM;
		private:
			// modalMix, modalSpreizung, modalHarmonize, modalSaturate
			void updateModalMix(double, double, double, double) noexcept;

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