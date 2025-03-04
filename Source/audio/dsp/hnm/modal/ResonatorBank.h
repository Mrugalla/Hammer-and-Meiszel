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
				ResoGain() :
					gains(),
					gain{ 1., 1. }
				{
					gains[0] = 1.;
					gains[1] = math::dbToAmp(24.);
					gains[2] = math::dbToAmp(32.);
					gains[NumGains] = gains[NumGains - 1];
				}

				void update(double x, int ch) noexcept
				{
					x *= MaxGainD;
					const auto xFloor = std::floor(x);
					const auto xFrac = x - xFloor;
					const auto i0 = static_cast<int>(xFloor);
					const auto i1 = i0 + 1;
					const auto g0 = gains[i0];
					const auto g1 = gains[i1];
					const auto gR = g1 - g0;
					gain[ch] = g0 + xFrac * gR;
				}

				const double operator()(int ch) const noexcept
				{
					return gain[ch];
				}
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

			// materialStereo, xen, noteNumber, numChannels
			void triggerNoteOn(const MaterialDataStereo&,
				const arch::XenManager&, double, int) noexcept;

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