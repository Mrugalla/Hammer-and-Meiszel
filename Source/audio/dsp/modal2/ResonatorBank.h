#pragma once
#include "../../../arch/XenManager.h"
#include "Material.h"
#include "../Resonator.h"

namespace dsp
{
	namespace modal2
	{
		class ResonatorBank
		{
			using ResonatorArray = std::array<ResonatorStereo2, NumFilters>;

			struct Val
			{
				Val();

				double getFreq(const arch::XenManager&) noexcept;

				double pitch, transpose, pb, pbRange, xen;
			};

		public:
			ResonatorBank();

			void reset() noexcept;

			// materialStereo, sampleRate
			void prepare(const MaterialDataStereo&, double);

			// materialStereo, samples, midi, xen, transposeSemi, numChannels, numSamples
			void operator()(const MaterialDataStereo&, double**, const MidiBuffer&,
				const arch::XenManager&, double, int, int) noexcept;

			// materialStereo, freqHz, numChannels
			void setFrequencyHz(const MaterialDataStereo&, double, int) noexcept;

			// materialStereo, numChannels
			void updateFreqRatios(const MaterialDataStereo&, int) noexcept;

			// bw, ch
			void setReso(double, int) noexcept;

		private:
			ResonatorArray resonators;
			Val val;
			double freqHz, sampleRate, sampleRateInv, nyquist, gain;
			std::array<int, 2> numFiltersBelowNyquist;

			// material, numFiltersBelowNyquist, ch
			void updateFreqRatios(const MaterialData&, int&, int) noexcept;

			/* materialStereo, samples, midi, xen, transposeSemi, numChannels, numSamples */
			void process(const MaterialDataStereo&, double**, const MidiBuffer&,
				const arch::XenManager&, double, int, int) noexcept;

			// materialStereo, samples, numChannels, startIdx, endIdx
			void applyFilter(const MaterialDataStereo&, double**,
				int, int, int) noexcept;
		};
	}
}

/*

*/