#include "FormantFilter.h"

namespace dsp
{
	namespace formant
	{
		// Param
		
		Param::Param(double _val, double _env, double _width) :
			val(_val),
			env(_env),
			width(_width)
		{
		}

		// Params

		Params::Params(double _blend, double _q,
			double blendEnv, double qEnv,
			double blendWidth, double qWidth) :
			blend(_blend, blendEnv, blendWidth),
			q(_q, qEnv, qWidth)
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

		// Voice

		Voice::Voice() :
			vowelStereo(),
			blendPRMs{ 0., 0. },
			qPRMs{ 0., 0. },
			resonators(),
			sleepy()
		{ }

		void Voice::prepare(double sampleRate) noexcept
		{
			for (auto& vowel : vowelStereo)
				vowel.prepare(sampleRate);
			for(auto& blend: blendPRMs)
				blend.prepare(sampleRate, 14.);
			for (auto& q : qPRMs)
				q.prepare(sampleRate, 14.);
			for (auto& resonator : resonators)
				resonator.reset();
			sleepy.prepare(sampleRate);
		}

		void Voice::operator()(double** samples,
			const Vowels& vowels, const Params& params, double envGenMod,
			int numChannels, int numSamples, bool forceUpdate) noexcept
		{
			updateParameters(vowels, params, envGenMod, numChannels, forceUpdate);
			resonate(samples, numChannels, numSamples);
		}

		void Voice::updateParameters(const Vowels& vowels, const Params& params, double envGenMod,
			int numChannels, bool forceUpdate) noexcept
		{
			double blendArray[2] =
			{
				params.blend.val + params.blend.env * envGenMod - params.blend.width,
				params.blend.val + params.blend.env * envGenMod + params.blend.width
			};

			double qArray[2] =
			{
				params.q.val + params.q.env * envGenMod - params.q.width,
				params.q.val + params.q.env * envGenMod + params.q.width
			};

			static constexpr auto QStart = .8;
			static constexpr auto QEnd = 0.1;
			static constexpr auto QRange = QEnd - QStart;
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto& blendPRM = blendPRMs[ch];
				auto& qPRM = qPRMs[ch];

				const auto blendLimited = math::limit(0., 1., blendArray[ch]);
				const auto blendInfo = blendPRM(blendLimited);

				const auto qLimited = math::limit(0., 1., qArray[ch]);
				const auto qMapped = QStart + qLimited * QRange;
				const auto qInfo = qPRM(qMapped * qMapped);

				if (forceUpdate || blendInfo.smoothing || qInfo.smoothing)
				{
					auto& vowel = vowelStereo[ch];
					vowel.blend(vowels[0], vowels[1], blendPRM.info.val);
					vowel.applyQ(qPRM.info.val);
					for (auto i = 0; i < NumFormants; ++i)
					{
						auto& resonator = resonators[i];
						const auto& formant = vowel.getFormant(i);
						resonator.setCutoffFc(formant.fc, ch);
						resonator.setBandwidth(formant.bwFc, ch);
						resonator.setGain(formant.gain, ch);
						resonator.update(ch);
					}
				}
			}
		}

		void Voice::triggerNoteOn() noexcept
		{
			sleepy.triggerNoteOn();
		}

		void Voice::triggerNoteOff() noexcept
		{
			sleepy.triggerNoteOff();
		}

		bool Voice::isSleepy() const noexcept
		{
			return sleepy.isSleepy();
		}

		void Voice::fallAsleepIfTired(double** samples, int numChannels, int numSamples) noexcept
		{
			sleepy(samples, numChannels, numSamples);
		}

		void Voice::resonate(double** samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				resonate(samples[ch], numSamples, ch);
		}

		void Voice::resonate(double* smpls, int numSamples, int ch) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dry = smpls[s];
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
			vowels
			{
				toVowel(VowelClass::SopranoA),
				toVowel(VowelClass::SopranoE)
			},
			envGens(),
			gainPRM(0.),
			voices(),
			decayMs(-1.),
			releaseMs(-1.),
			wannaUpdate(false)
		{
		}

		void Filter::prepare(double sampleRate) noexcept
		{
			for (auto& vowel : vowels)
				vowel.prepare(sampleRate);
			envGens.prepare(sampleRate);
			gainPRM.prepare(sampleRate, 7.);
			for (auto& voice : voices)
				voice.prepare(sampleRate);
			decayMs = -1.;
			releaseMs = -1.;
			wannaUpdate = false;
		}

		void Filter::updateParameters(double _attackMs, double _decayMs, double _releaseMs, double gainDb,
			VowelClass vowelClassA, VowelClass vowelClassB) noexcept
		{
			if (attackMs != _attackMs || decayMs != _decayMs || releaseMs != _releaseMs)
			{
				attackMs = _attackMs;
				decayMs = _decayMs;
				releaseMs = _releaseMs;
				envGens.updateParametersMs({ attackMs, decayMs, 0., releaseMs });
			}

			const auto gain = math::dbToAmp(gainDb, -60.);
			gainPRM(gain);
			
			wannaUpdate = false;
			if (vowels[0].getVowelClass() != vowelClassA)
			{
				vowels[0] = toVowel(vowelClassA);
				wannaUpdate = true;
			}
			if (vowels[1].getVowelClass() != vowelClassB)
			{
				vowels[1] = toVowel(vowelClassB);
				wannaUpdate = true;
			}
		}

		void Filter::operator()(double** samples, const Params& params, double envGenMod,
			int numChannels, int numSamples, int v) noexcept
		{
			auto& voice = voices[v];
			voice(samples, vowels, params, envGenMod, numChannels, numSamples, wannaUpdate);
			if (envGens.processGain(samples, numChannels, numSamples, v))
				for (auto ch = 0; ch < numChannels; ++ch)
					SIMD::multiply(samples[ch], gainPRM.info.val, numSamples);
			voice.fallAsleepIfTired(samples, numChannels, numSamples);
		}

		void Filter::triggerNoteOn(int v) noexcept
		{
			auto& voice = voices[v];
			voice.triggerNoteOn();
			envGens.triggerNoteOn(true, v);
		}

		void Filter::triggerNoteOff(int v) noexcept
		{
			auto& voice = voices[v];
			voice.triggerNoteOff();
			envGens.triggerNoteOn(false, v);
		}

		bool Filter::isRinging(int v) const noexcept
		{
			auto& voice = voices[v];
			return !voice.isSleepy();
		}
	}
}