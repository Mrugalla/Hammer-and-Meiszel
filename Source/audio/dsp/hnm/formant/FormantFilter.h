#pragma once
#include "../../../../arch/XenManager.h"
#include "../../Resonator.h"
#include "FormantAxiom.h"
#include "../../PRM.h"
#include "../../SleepyDetector.h"
#include "../../EnvelopeGenerator.h"

namespace dsp
{
	namespace formant
	{
		struct Param
		{
			Param(double, double, double);
			double val, env, width;
		};

		struct Params
		{
			//blend, q, blendEnv, qEnv, blendWidth, qWidth
			Params(double = 0., double = 0., double = 0.,
				double = 0., double = 0., double = 0.);
			Param blend, q;
		};

		// https://www.classes.cs.uchicago.edu/archive/1999/spring/CS295/Computing_Resources/Csound/CsManual3.48b1.HTML/Appendices/table3.html

		struct ParamStereo
		{
			ParamStereo(double = 0.);

			void prepare(double) noexcept;

			// val, ch, numSamples
			PRMInfoD operator()(double, int, int) noexcept;

			// ch
			PRMD& operator[](int) noexcept;

			// ch
			const PRMD& operator[](int) const noexcept;
		protected:
			std::array<PRMD, 2> prms;
		};

		using Vowels = std::array<Vowel, 2>;
		
		class Voice
		{
			using ResonatorArray = std::array<ResonatorStereo2, NumFormants>;
		public:
			Voice();

			// sampleRate
			void prepare(double) noexcept;

			// samples, vowels, params, envGenMod, numChannels, numSamples, forceUpdate
			void operator()(double**, const Vowels&, const Params&, double, int, int, bool) noexcept;

			void triggerNoteOn() noexcept;

			void triggerNoteOff() noexcept;

			bool isSleepy() const noexcept;

			// samples, numChannels, numSamples
			void fallAsleepIfTired(double**, int, int) noexcept;
		private:
			Vowels vowelStereo;
			std::array<PRMBlockD, 2> blendPRMs, qPRMs;
			ResonatorArray resonators;
			SleepyDetector sleepy;

			// vowels, params, envGenMod, numChannels, forceUpdate
			void updateParameters(const Vowels&, const Params&, double, int, bool) noexcept;

			// samples, numChannels, numSamples
			void resonate(double**, int, int) noexcept;

			// smpls, numSamples, ch
			void resonate(double*, int, int) noexcept;
		};

		struct Filter
		{
			Filter();

			// sampleRate
			void prepare(double) noexcept;

			// attackMs, decayMs, relaseMs, gainDb, vowelClassA, vowelClassB
			void updateParameters(double, double, double, double, VowelClass, VowelClass) noexcept;

			// samples, params, envGenMod, numChannels, numSamples, v
			void operator()(double**, const Params&, double, int, int, int) noexcept;

			void triggerNoteOn(int) noexcept;

			void triggerNoteOff(int) noexcept;

			bool isRinging(int) const noexcept;
		private:
			Vowels vowels;
			EnvGenMultiVoice envGens;
			PRMBlockD gainPRM;
			std::array<Voice, NumMPEChannels> voices;
			double attackMs, decayMs, releaseMs;
			bool wannaUpdate;
		};
	}
}