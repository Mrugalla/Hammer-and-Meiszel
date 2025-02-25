#pragma once
#include "../../../../arch/XenManager.h"
#include "Material.h"
#include "../../Resonator.h"
#include "../../SleepyDetector.h"

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

				double pitch, transpose, pb;
			};

		public:
			ResonatorBank();

			void reset() noexcept;

			// materialStereo, sampleRate
			void prepare(const MaterialDataStereo&, double);

			// materialStereo, samples, numChannels, numSamples
			void operator()(const MaterialDataStereo&, double**, int, int) noexcept;

			// materialStereo, xen, transposeSemi, numChannels
			void setTranspose(const MaterialDataStereo&,
				const arch::XenManager&, double, int) noexcept;

			// materialStereo, samples, numChannels, numSamples
			void applyFilter(const MaterialDataStereo&, double**,
				int, int) noexcept;

			// xen, materialStereo, numChannels
			void triggerXen(const arch::XenManager&,
				const MaterialDataStereo&, int) noexcept;

			// materialStereo, xen, noteNumber, numChannels
			void triggerNoteOn(const MaterialDataStereo&,
				const arch::XenManager&, double, int) noexcept;

			void triggerNoteOff() noexcept;

			// materialStereo, xen, pitchbend, numChannels
			void triggerPitchbend(const MaterialDataStereo&, const arch::XenManager&,
				double, int) noexcept;

			// materialStereo, freqHz, numChannels
			void setFrequencyHz(const MaterialDataStereo&, double, int) noexcept;

			// materialStereo, numChannels
			void updateFreqRatios(const MaterialDataStereo&, int) noexcept;

			// bw, ch
			void setReso(double, int) noexcept;

			bool isRinging() const noexcept;

		private:
			ResonatorArray resonators;
			Val val;
			const double FreqMin;
			double freqMax, freqHz, sampleRate, sampleRateInv, nyquist;
			std::array<double, 2> gains;
			std::array<int, 2> numFiltersBelowNyquist;
			SleepyDetector sleepy;

			// material, numFiltersBelowNyquist, ch
			void updateFreqRatios(const MaterialData&, int&, int) noexcept;
		};
	}
}