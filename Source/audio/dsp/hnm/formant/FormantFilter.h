#pragma once
#include "../../../../arch/XenManager.h"
#include "../../Resonator.h"
#include "FormantAxiom.h"
#include "../../PRM.h"
#include "../../SleepyDetector.h"

namespace dsp
{
	namespace formant
	{
		struct Params
		{
			//vowelPos, q, gainDb, vowelPosMod, qMod, gainDbMod, vowelA, vowelB
			Params(double, double, double,
				double, double, double,
				int, int);

			double vowelPos, q, gain, vowelPosMod, qMod, gainMod;
			int vowelA, vowelB;
		};

		// https://www.classes.cs.uchicago.edu/archive/1999/spring/CS295/Computing_Resources/Csound/CsManual3.48b1.HTML/Appendices/table3.html

		struct DualVowel
		{
			DualVowel();

			// sampleRate
			void prepare(double) noexcept;

			// params, envGenMod
			bool wannaUpdate(const Params&, double) noexcept;

			const Vowel& getVowel() const noexcept;

		private:
			Vowel a, b, ab;
			Params params;
		};

		class Voice
		{
			using ResonatorBank = std::array<ResonatorStereo2, NumFormants>;
		public:
			Voice();

			void prepare(double) noexcept;

			// samples, params, envGenMod, numChannels, numSamples
			void operator()(double**, const Params&, double, int, int) noexcept;

			void triggerNoteOn() noexcept;

			void triggerNoteOff() noexcept;

			// params, envGenMod
			void updateParameters(const Params&, double) noexcept;

			bool isSleepy() const noexcept;
		private:
			ResonatorBank resonators;
			DualVowel dualVowel;
			PRMD gainPRM;
			SleepyDetector sleepyDetector;

			void updateResonators(const Vowel&) noexcept;

			// params, envGenMod, numSamples
			void updateGain(const Params&, double, int) noexcept;

			// samples, numChannels, numSamples
			void resonate(double**, int, int) noexcept;

			// smpls, numSamples, ch
			void resonate(double*, int, int) noexcept;

			// samples, params, envGenMod, numChannels, numSamples
			void process(double**, const Params&,
				double, int, int) noexcept;
		};

		struct Filter
		{
			Filter();

			// sampleRate
			void prepare(double) noexcept;

			// samples, params, envGenMod, numChannels, numSamples, v
			void operator()(double**, const Params&,
				double, int, int, int) noexcept;

			void triggerNoteOn(int) noexcept;

			void triggerNoteOff(int) noexcept;

			bool isRinging(int) const noexcept;
		private:
			
			std::array<Voice, NumMPEChannels> voices;
		};
	}
}