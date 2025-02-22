#include "FormantFilter.h"

namespace dsp
{
	namespace formant
	{
		// Params

		Params::Params(double _vowelPos, double _q, double _gainDb,
			double _vowelPosMod, double _qMod, double _gainMod,
			int _vowelA, int _vowelB) :
			vowelPos(_vowelPos), q(_q), gain(_gainDb),
			vowelPosMod(_vowelPosMod), qMod(_qMod), gainMod(_gainMod),
			vowelA(_vowelA), vowelB(_vowelB)
		{
		}

		// DualVowel

		DualVowel::DualVowel() :
			a(toVowel(VowelClass::SopranoA)),
			b(toVowel(VowelClass::SopranoE)),
			params(-1., -1., -120., 0., 0., 0., -1, -1)
		{
		}

		void DualVowel::prepare(double sampleRate) noexcept
		{
			a.prepare(sampleRate);
			b.prepare(sampleRate);
			ab.prepare(sampleRate);
			params = Params(-1., -1., -120., 0., 0., 0., -1, -1);
		}

		bool DualVowel::wannaUpdate(const Params& _params, double envGenMod) noexcept
		{
			bool update = false;

			if (params.vowelA != _params.vowelA)
			{
				params.vowelA = _params.vowelA;
				const auto vowelClass = static_cast<VowelClass>(params.vowelA);
				a = toVowel(vowelClass);
				update = true;
			}

			if (params.vowelB != _params.vowelB)
			{
				params.vowelB = _params.vowelB;
				const auto vowelClass = static_cast<VowelClass>(params.vowelB);
				b = toVowel(vowelClass);
				update = true;
			}

			const auto vowelPos = math::limit(0., 1., _params.vowelPos + _params.vowelPosMod * envGenMod);
			const auto q = math::limit(0., 1., _params.q + _params.qMod * envGenMod);

			if (params.vowelPos != vowelPos ||
				params.q != q)
			{
				params.vowelPos = vowelPos;
				params.q = q;
				update = true;
			}

			if (update)
				ab.blend(a, b, params.vowelPos, params.q);
			return update;
		}

		const Vowel& DualVowel::getVowel() const noexcept
		{
			return ab;
		}

		// Voice

		Voice::Voice() :
			resonators(),
			dualVowel(),
			gainPRM(0.),
			sleepyDetector()
		{
		}

		void Voice::prepare(double sampleRate) noexcept
		{
			for (auto& resonator : resonators)
				resonator.reset();
			dualVowel.prepare(sampleRate);
			gainPRM.prepare(sampleRate, 7.);
			sleepyDetector.prepare(sampleRate);
		}

		void Voice::updateResonators(const Vowel& vowel) noexcept
		{
			for (auto i = 0; i < NumFormants; ++i)
			{
				auto& resonator = resonators[i];
				const auto& formant = vowel.getFormant(i);
				resonator.setBandwidth(formant.bwFc, 0);
				resonator.setCutoffFc(formant.fc, 0);
				resonator.setGain(formant.gain, 0);
				resonator.update();
			}
		}

		void Voice::updateParameters(const Params& params, double envGenMod) noexcept
		{
			if (!dualVowel.wannaUpdate(params, envGenMod))
				return;
			const auto& vowel = dualVowel.getVowel();
			updateResonators(vowel);
		}

		void Voice::operator()(double** samples, const Params& params,
			double envGenMod, int numChannels, int numSamples) noexcept
		{
			updateParameters(params, envGenMod);
			process(samples, params, envGenMod, numChannels, numSamples);
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

		void Voice::updateGain(const Params& params, double envGenMod, int numSamples) noexcept
		{
			const auto gainDb = params.gain + params.gainMod * envGenMod;
			const auto gain = math::dbToAmp(gainDb, -60.);
			auto gainInfo = gainPRM(gain, numSamples);
			gainInfo.copyToBuffer(numSamples);
		}

		void Voice::resonate(double** samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				resonate(samples[ch], numSamples, ch);
			sleepyDetector(samples, numChannels, numSamples);
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
			SIMD::multiply(smpls, gainPRM.buf.data(), numSamples);
		}

		void Voice::process(double** samples, const Params& params,
			double envGenMod, int numChannels, int numSamples) noexcept
		{
			//oopsie(!sleepyDetector.isNoteOn() && sleepyDetector.isRinging());
			updateGain(params, envGenMod, numSamples);
			resonate(samples, numChannels, numSamples);
		}

		// FormantFilter

		Filter::Filter() :
			voices()
		{
		}

		void Filter::prepare(double sampleRate) noexcept
		{
			for (auto& voice : voices)
				voice.prepare(sampleRate);
		}

		void Filter::operator()(double** samples, const Params& params,
			double envGenMod, int numChannels, int numSamples, int v) noexcept
		{
			voices[v](samples, params, envGenMod, numChannels, numSamples);
		}

		void Filter::triggerNoteOn(int v) noexcept
		{
			voices[v].triggerNoteOn();
		}

		void Filter::triggerNoteOff(int v) noexcept
		{
			voices[v].triggerNoteOff();
		}

		bool Filter::isRinging(int v) const noexcept
		{
			return !voices[v].isSleepy();
		}
	}
}