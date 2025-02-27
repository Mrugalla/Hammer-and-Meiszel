#include "FormantFilter.h"

namespace dsp
{
	namespace formant
	{
		// Params

		Params::Params(double _vowelPos, double _q,
			double vowelPosEnv, double qEnv,
			double vowelPosWidth, double qWidth,
			int _vowelA, int _vowelB) :
			vowelPos(_vowelPos, vowelPosEnv, vowelPosWidth),
			q(_q, qEnv, qWidth),
			vowelA(_vowelA), vowelB(_vowelB)
		{
		}

		// ParamStereo

		ParamStereo::ParamStereo(double v) :
			prms{ v, v }
		{
		}

		void ParamStereo::ParamStereo::prepare(double sampleRate) noexcept
		{
			for (auto& prm : prms)
				prm.prepare(sampleRate, 13.);
		}

		PRMInfoD ParamStereo::operator()(double val, int ch, int numSamples) noexcept
		{
			return prms[ch](val, numSamples);
		}

		PRMD& ParamStereo::operator[](int ch) noexcept
		{
			return prms[ch];
		}

		const PRMD& ParamStereo::operator[](int ch) const noexcept
		{
			return prms[ch];
		}

		// DualVowel

		DualVowel::DualVowel() :
			a(toVowel(VowelClass::SopranoA)),
			b(toVowel(VowelClass::SopranoE)),
			ab(),
			vowelPos(),
			vowelA(0), vowelB(0)
		{
		}

		void DualVowel::prepare(double sampleRate) noexcept
		{
			a.prepare(sampleRate);
			b.prepare(sampleRate);
			ab.prepare(sampleRate);
			vowelPos.prepare(sampleRate);
			vowelA = 0;
			vowelB = 0;
		}

		bool DualVowel::wannaUpdate(const Params& params,
			double envGenMod, int numChannels, int numSamples) noexcept
		{
			bool update = false;
			updateVowels(params, update);
			updateVowelPos(params, envGenMod, update, numChannels, numSamples);
			return update;
		}

		VowelStereo& DualVowel::getVowel() noexcept
		{
			return ab;
		}

		void DualVowel::updateVowels(const Params& params, bool& update) noexcept
		{
			if (vowelA != params.vowelA)
			{
				vowelA = params.vowelA;
				const auto vowelClass = static_cast<VowelClass>(params.vowelA);
				a = toVowel(vowelClass);
				update = true;
			}

			if (vowelB != params.vowelB)
			{
				vowelB = params.vowelB;
				const auto vowelClass = static_cast<VowelClass>(params.vowelB);
				b = toVowel(vowelClass);
				update = true;
			}
		}

		void DualVowel::updateVowelPos(const Params& params, double envGenMod,
			bool& update, int numChannels, int numSamples) noexcept
		{
			double vowelArray[2] =
			{
				params.vowelPos.val + params.vowelPos.env * envGenMod + params.vowelPos.width,
				params.vowelPos.val + params.vowelPos.env * envGenMod - params.vowelPos.width
			};
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto posCh = math::limit(0., 1., vowelArray[ch]);
				const auto vowelPosInfo = vowelPos(posCh, ch, numSamples);
				if (vowelPosInfo.smoothing)
				{
					ab.blend(a, b, vowelPos[ch].value, ch);
					update = true;
				}
			}
		}

		// Voice

		Voice::Voice() :
			resonators(),
			dualVowel(),
			sleepyDetector(),
			qPRMs(0.),
			qUpdateFuncs
			{
				[&](int ch, int s)
				{
					const auto& qPRM = qPRMs[ch];
					const auto qVal = qPRM[s];
					auto& vowel = dualVowel.getVowel();
					vowel.applyQ(qVal, ch);
					for (auto& resonator : resonators)
					{
						resonator.setBandwidth(qVal, ch);
						resonator.update(ch);
					}
				},
				[](int, int){}
			}
		{
		}

		void Voice::prepare(double sampleRate) noexcept
		{
			for (auto& resonator : resonators)
				resonator.reset();
			dualVowel.prepare(sampleRate);
			sleepyDetector.prepare(sampleRate);
			qPRMs.prepare(sampleRate);
		}

		void Voice::updateResonatorsBlock(const VowelStereo& vowel, int numChannels) noexcept
		{
			for(auto ch = 0; ch < numChannels; ++ch)
				for (auto i = 0; i < NumFormants; ++i)
				{
					auto& resonator = resonators[i];
					const auto& formant = vowel.getFormant(i, ch);
					resonator.setCutoffFc(formant.fc, ch);
					resonator.setGain(formant.gain, ch);
					resonator.update(ch);
				}
		}

		void Voice::updateParameters(const Params& params, double envGenMod,
			int numChannels, int numSamples) noexcept
		{
			if (!dualVowel.wannaUpdate(params, envGenMod, numChannels, numSamples))
				return;
			const auto& vowel = dualVowel.getVowel();
			updateResonatorsBlock(vowel, numChannels);
		}

		void Voice::updateQ(const Params& params, double envGenMod,
			int numChannels, int numSamples) noexcept
		{
			double qArray[2] =
			{
				params.q.val + params.q.env * envGenMod + params.q.width,
				params.q.val + params.q.env * envGenMod - params.q.width
			};
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto val = math::limit(0., 1., qArray[ch]);
				static constexpr auto QStart = .9;
				static constexpr auto QEnd = .1;
				static constexpr auto QRange = QEnd - QStart;
				val = QStart + val * QRange;
				val *= val;
				qPRMs(val, ch, numSamples);
			}
		}

		void Voice::operator()(double** samples, const Params& params,
			double envGenMod, int numChannels, int numSamples) noexcept
		{
			updateParameters(params, envGenMod, numChannels, numSamples);
			updateQ(params, envGenMod, numChannels, numSamples);
			resonate(samples, numChannels, numSamples);
		}

		void Voice::triggerNoteOn() noexcept
		{
			sleepyDetector.triggerNoteOn();
		}

		void Voice::triggerNoteOff() noexcept
		{
			sleepyDetector.triggerNoteOff();
		}

		bool Voice::isSleepy() const noexcept
		{
			return sleepyDetector.isSleepy();
		}

		void Voice::resonate(double** samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				resonate(samples[ch], numSamples, ch);
			sleepyDetector(samples, numChannels, numSamples);
		}

		void Voice::resonate(double* smpls, int numSamples, int ch) noexcept
		{
			const auto& qPRM = qPRMs[ch];
			const auto& qUpdateFunc = qPRM.smoothing ? qUpdateFuncs[0] : qUpdateFuncs[1];

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dry = smpls[s];
				qUpdateFunc(ch, s);
				auto y = resonators[0](dry, ch);
				for (auto i = 1; i < NumFormants; ++i)
				{
					auto& resonator = resonators[i];
					y += resonator(dry, ch);
				}
				smpls[s] = y;
			}
		}

		// FormantFilter

		Filter::Filter() :
			envGens(),
			gainPRM(0.),
			voices(),
			decayMs(-1.),
			releaseMs(-1.)
		{
		}

		void Filter::prepare(double sampleRate) noexcept
		{
			decayMs = -1.;
			releaseMs = -1.;
			gainPRM.prepare(sampleRate, 7.);
			for (auto& voice : voices)
				voice.prepare(sampleRate);
			envGens.prepare(sampleRate);
		}

		void Filter::updateEnvelopes(double _decayMs, double _releaseMs,
			double gainDb, int numSamples) noexcept
		{
			if (decayMs != _decayMs || releaseMs != _releaseMs)
			{
				decayMs = _decayMs;
				releaseMs = _releaseMs;
				envGens.updateParameters(EnvelopeGenerator::Parameters(0., decayMs, 0., releaseMs));
			}

			const auto gain = math::dbToAmp(gainDb, -60.);
			auto gainInfo = gainPRM(gain, numSamples);
		}

		void Filter::operator()(double** samples, const Params& params,
			double envGenMod, int numChannels, int numSamples, int v) noexcept
		{
			voices[v](samples, params, envGenMod, numChannels, numSamples);
			if (envGens.processGain(samples, numChannels, numSamples, v))
				for (auto ch = 0; ch < numChannels; ++ch)
					SIMD::multiply(samples[ch], gainPRM.value, numSamples);
		}

		void Filter::triggerNoteOn(int v) noexcept
		{
			voices[v].triggerNoteOn();
			envGens.triggerNoteOn(true, v);
		}

		void Filter::triggerNoteOff(int v) noexcept
		{
			voices[v].triggerNoteOff();
			envGens.triggerNoteOn(false, v);
		}

		bool Filter::isRinging(int v) const noexcept
		{
			return !voices[v].isSleepy();
		}
	}
}