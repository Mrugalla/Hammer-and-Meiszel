#pragma once
#include "../../../../arch/XenManager.h"
#include "Material.h"
#include "../../Resonator.h"

namespace dsp
{
	namespace modal
	{
		class ResonatorBank
		{
			using ResonatorArray = std::array<ResonatorStereo2, NumPartials>;

			struct Val
			{
				Val();

				void reset() noexcept;

				double getFreq(const arch::XenManager&) noexcept;

				double pitch, transpose, pb, pbRange, xen, masterTune, anchor;
			};

		public:
			ResonatorBank();

			// numChannels
			void reset(int) noexcept;

			// materialStereo, sampleRate
			void prepare(const MaterialDataStereo&, double);

			// materialStereo, samples, midi, xen, transposeSemi, numChannels, numSamples
			void operator()(const MaterialDataStereo&, double**, const MidiBuffer&,
				const arch::XenManager&, double, int, int) noexcept;

			// materialStereo, freqHz, numChannels
			void setFrequencyHz(const MaterialDataStereo&, double, int) noexcept;

			// materialStereo, numChannels
			void updateFreqRatios(const MaterialDataStereo&, int) noexcept;

			// bw, damp, ch
			void setReso(double, double, int) noexcept;

			bool isRinging() const noexcept;

		private:
			ResonatorArray resonators;
			Val val;
			const double FreqMin;
			double freqMax, freqHz, sampleRate, sampleRateInv, nyquist;
			std::array<double, 2> gains;
			std::array<int, 2> numFiltersBelowNyquist;
			int sleepyTimer, sleepyTimerThreshold;
			bool ringing;

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