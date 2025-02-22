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

					// xen, params, envGenMod, numSamples
					void operator()(const arch::XenManager&,
						const Params&, double, int) noexcept;

					// xen, params, envGenMod
					void updateDamp(const arch::XenManager&,
						const Params&, double) noexcept;

					// xen
					void updateXen(const arch::XenManager&) noexcept;

					// xen, note
					void updatePitch(const arch::XenManager&, double) noexcept;

					// xen
					void updateFreqHz(const arch::XenManager&) noexcept;

					bool isSmoothing() const noexcept;

					double operator[](int) const noexcept;

				private:
					double pitch, damp, freqHz;
					arch::XenManager::Info xenInfo;
					PRMD prm;
					PRMInfoD info;
				};

				Voice();

				// sampleRate
				void prepare(double) noexcept;

				// samples, params, xen, envGenMod, numChannels, numSamples
				void operator()(double**,
					const Params&, const arch::XenManager&,
					double, int, int) noexcept;

				// xen, noteNumber
				void triggerNoteOn(const arch::XenManager&, double) noexcept;

				void triggerNoteOff() noexcept;

				// xen, pitchbend
				void triggerPitchbend(const arch::XenManager&, double) noexcept;

				bool isRinging() const noexcept;
			private:
				std::array<std::function<void(double)>, 2> updateCutoffFuncs;
				LP lp;
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

				// xen, noteNumber, v
				void triggerNoteOn(const arch::XenManager&, double, int) noexcept;

				// v
				void triggerNoteOff(int) noexcept;

				// xen, pitchbend, v
				void triggerPitchbend(const arch::XenManager&, double, int) noexcept;

				bool isRinging(int) const noexcept;
			private:
				std::array<Voice, NumMPEChannels> voices;
			};
		}
	}
}