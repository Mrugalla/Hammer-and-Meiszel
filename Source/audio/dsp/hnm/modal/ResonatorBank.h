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

			class ResoGain
			{
				static constexpr int NumGains = 3;
				static constexpr int MaxGain = NumGains - 1;
				static constexpr double MaxGainD = static_cast<double>(MaxGain);
			public:
				ResoGain();

				// x, ch
				void update(double, int) noexcept;

				// ch
				double operator()(int) const noexcept;
			private:
				std::array<double, NumGains + 1> gains;
				std::array<double, 2> gain;
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

			// materialStereo, xen, noteNumber, numChannels, polyphonic
			void triggerNoteOn(const MaterialDataStereo&,
				const arch::XenManager&, double, int, bool) noexcept;

			void triggerNoteOff() noexcept;

			// materialStereo, xen, pitchbend, numChannels
			void triggerPitchbend(const MaterialDataStereo&, const arch::XenManager&,
				double, int) noexcept;

			// materialStereo, freqHz, numChannels
			bool setFrequencyHz(const MaterialDataStereo&, double, int) noexcept;

			// materialStereo, numChannels
			void updateFreqRatios(const MaterialDataStereo&, int) noexcept;

			// bw, ch
			void setReso(double, int) noexcept;

			bool isRinging() const noexcept;

		private:
			ResonatorArray resonators;
			Val val;
			double freqHz, sampleRate, sampleRateInv, nyquist;
			ResoGain autoGainReso;
			std::array<int, 2> numFiltersBelowNyquist;
			SleepyDetector sleepy;

			// material, numFiltersBelowNyquist, ch
			void updateFreqRatios(const MaterialData&, int&, int) noexcept;
		};
	}
}