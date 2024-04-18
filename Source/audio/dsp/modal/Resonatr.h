#pragma once
#include "../../../arch/XenManager.h"
#include "Material.h"
#include "../Resonator.h"

namespace dsp
{
    namespace modal
    {
		using ResonatorArray = std::array<ResonatorStereo2, NumFilters>;

		struct Resonatr
		{
			struct Val
			{
				Val();

				double getFreq(const arch::XenManager&) noexcept;

				double pitch, transpose, pb, pbRange, xen;
			};

			Resonatr(const arch::XenManager&, const PeakArray&);

			void reset() noexcept;

			void prepare(double);

			// samples, midi, numChannels, numSamples
			void operator()(double**, const MidiBuffer&, int, int) noexcept;

			// freqHz
			void setFrequencyHz(double) noexcept;

			// transposeSemi, pbRange, xen
			void updateVal(double transposeSemi, double pbRange, double _xen) noexcept
			{
				val.transpose = transposeSemi;
				val.pbRange = pbRange;
				val.xen = _xen;
				const auto freq = val.getFreq(xen);
				setFrequencyHz(freq);
			}

			void updateFreqRatios() noexcept;

			// bw
			void setBandwidth(double) noexcept;

		private:
			const arch::XenManager& xen;
			//const DualMaterial& material;
			const PeakArray& peaks;
			ResonatorArray resonators;
			Val val;
			double freqHz, sampleRate, nyquist;
			int numFiltersBelowNyquist;

			/* samples, midi, numChannels, numSamples */
			void process(double**, const MidiBuffer&, int, int) noexcept;

			// samples, numChannels, startIdx, endIdx
			void applyFilter(double**, int, int, int) noexcept;
		};
    }
}