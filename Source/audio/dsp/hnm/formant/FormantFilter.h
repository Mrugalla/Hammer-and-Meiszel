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
			Param(double _val, double _env, double _width) :
				val(_val),
				env(_env),
				width(_width)
			{
			}
			double val, env, width;
		};

		struct Params
		{
			//vowelPos, q, vowelPosEnv, qEnv, vowelPosWidth, qWidth, vowelA, vowelB
			Params(double = 0., double = 0., double = 0.,
				double = 0., double = 0., double = 0.,
				int = 0, int = 0);

			Param vowelPos, q;
			int vowelA, vowelB;
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

		struct DualVowel
		{
			DualVowel();

			// sampleRate
			void prepare(double) noexcept;

			// params, envGenMod, numChannels, numSamples
			bool wannaUpdate(const Params&, double, int, int) noexcept;

			VowelStereo& getVowel() noexcept;
		private:
			Vowel a, b;
			VowelStereo ab;
			ParamStereo vowelPos;
			int vowelA, vowelB;

			// params, update
			void updateVowels(const Params&, bool&) noexcept;

			// params, envGenMod, update, numChannels, numSamples
			void updateVowelPos(const Params&, double, bool&, int, int) noexcept;
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

			// params, envGenMod, numChannels, numSamples
			void updateParameters(const Params&, double, int, int) noexcept;

			bool isSleepy() const noexcept;
		private:
			ResonatorBank resonators;
			DualVowel dualVowel;
			SleepyDetector sleepyDetector;
			ParamStereo qPRMs;
			std::array<std::function<void(int, int)>, 2> qUpdateFuncs;

			// vowel, numChannels
			void updateResonatorsBlock(const VowelStereo&, int) noexcept;

			// params, envGenMod, numChannels, numSamples
			void updateQ(const Params&, double, int, int) noexcept;

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

			// decayMs, relaseMs, gainDb, numSamples
			void updateEnvelopes(double, double, double, int) noexcept;

			// samples, params, envGenMod, numChannels, numSamples, v
			void operator()(double**, const Params&,
				double, int, int, int) noexcept;

			void triggerNoteOn(int) noexcept;

			void triggerNoteOff(int) noexcept;

			bool isRinging(int) const noexcept;
		private:
			EnvGenMultiVoice envGens;
			PRMD gainPRM;
			std::array<Voice, NumMPEChannels> voices;
			double decayMs, releaseMs;
		};
	}
}