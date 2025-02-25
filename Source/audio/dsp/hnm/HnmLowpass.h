#pragma once
#include "../PRM.h"
#include "../SleepyDetector.h"
#include "../../../arch/XenManager.h"
#include <juce_dsp/juce_dsp.h>

namespace dsp
{
	namespace hnm
	{
		namespace lp
		{
			struct Params
			{
				// damp, dampEnv, dampWidth
				Params(double = 0., double = 0., double = 0.);

				double damp, dampEnv, dampWidth;
			};

			struct Voice
			{
				using LP = juce::dsp::StateVariableTPTFilter<double>;

				struct Val
				{
					Val();

					// sampleRate
					void prepare(double) noexcept;

					// xen, params, envGenMod, numChannels, numSamples
					void operator()(const arch::XenManager&,
						const Params&, double, int, int) noexcept;

					// xen, numChannels
					void updateXen(const arch::XenManager&, int) noexcept;

					// xen, note, numChannels
					void updatePitch(const arch::XenManager&, double, int) noexcept;

					// xen, pitchbend, numChannels
					void updatePitchbend(const arch::XenManager&, double, int) noexcept;

					// xen, ch
					void updateFreqHz(const arch::XenManager&, int) noexcept;

					// ch
					bool isSmoothing(int) const noexcept;

					// ch, s
					double operator()(int, int) const noexcept;

				private:
					double pitch, pitchbend;
					std::array<double, 2> damps;
					std::array<PRMD, 2> prms;
				};

				Voice();

				// sampleRate
				void prepare(double) noexcept;

				// samples, params, xen, envGenMod, numChannels, numSamples
				void operator()(double**,
					const Params&, const arch::XenManager&,
					double, int, int) noexcept;

				// xen, numChannels
				void triggerXen(const arch::XenManager&, int) noexcept;

				// xen, noteNumber, numChannels
				void triggerNoteOn(const arch::XenManager&, double, int) noexcept;

				void triggerNoteOff() noexcept;

				// xen, pitchbend, numChannels
				void triggerPitchbend(const arch::XenManager&, double, int) noexcept;

				bool isRinging() const noexcept;
			private:
				std::array<std::function<void(double, int)>, 2> updateCutoffFuncs;
				std::array<LP, 2> lps;
				Val val;
				SleepyDetector sleepy;

				// samples, numChannels, numSamples
				void process(double**, int, int) noexcept;
			};

			struct Filter
			{
				Filter();

				// sampleRate
				void prepare(double) noexcept;

				// samples, params, xen, envGenMod, numChannels, numSamples, v
				void operator()(double**,
					const Params&, const arch::XenManager&,
					double, int, int, int) noexcept;

				// xen, numChannels
				void triggerXen(const arch::XenManager&, int) noexcept;

				// xen, noteNumber, numChannels, v
				void triggerNoteOn(const arch::XenManager&, double, int, int) noexcept;

				// v
				void triggerNoteOff(int) noexcept;

				// xen, pitchbend, numChannels, v
				void triggerPitchbend(const arch::XenManager&, double, int, int) noexcept;

				bool isRinging(int) const noexcept;
			private:
				std::array<Voice, NumMPEChannels> voices;
			};
		}
	}
}