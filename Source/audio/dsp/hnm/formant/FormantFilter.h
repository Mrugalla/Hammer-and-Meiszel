#pragma once
#include "../../../../arch/XenManager.h"
#include "../../Resonator.h"
#include "Axiom.h"

namespace dsp
{
	namespace formant
	{
		struct Params
		{
			Params(double _vowelPos, double _q, double _gainDb, int _vowelA, int _vowelB) :
				vowelPos(_vowelPos), q(_q), gain(_gainDb),
				vowelA(_vowelA), vowelB(_vowelB)
			{
			}

			double vowelPos, q, gain;
			int vowelA, vowelB;
		};

		// https://www.classes.cs.uchicago.edu/archive/1999/spring/CS295/Computing_Resources/Csound/CsManual3.48b1.HTML/Appendices/table3.html

		struct DualVowel
		{
			DualVowel() :
				a(toVowel(VowelClass::SopranoA)),
				b(toVowel(VowelClass::SopranoE)),
				params(-1., -1., -120., -1, -1)
			{
			}

			void prepare(double sampleRate) noexcept
			{
				a.prepare(sampleRate);
				b.prepare(sampleRate);
				ab.prepare(sampleRate);
				params = Params(-1., -1., -120., -1, -1);
			}

			bool wannaUpdate(const Params& _params) noexcept
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

				if (params.vowelPos != _params.vowelPos ||
					params.q != _params.q)
				{
					params.vowelPos = _params.vowelPos;
					params.q = _params.q;
					update = true;
				}
				
				if(update)
					ab.blend(a, b, params.vowelPos, params.q);
				return update;
			}

			const Vowel& getVowel() const noexcept
			{
				return ab;
			}

		private:
			Vowel a, b, ab;
			Params params;
		};

		struct Voice
		{
			using ResonatorBank = std::array<ResonatorStereo2, NumFormants>;

			Voice() :
				resonators()
			{
			}

			void prepare() noexcept
			{
				for (auto& resonator : resonators)
					resonator.reset();
			}

			void updateResonators(const Vowel& vowel) noexcept
			{
				for(auto i = 0; i < NumFormants; ++i)
				{
					auto& resonator = resonators[i];
					const auto& formant = vowel.getFormant(i);
					resonator.setBandwidth(formant.bwFc, 0);
					resonator.setCutoffFc(formant.fc, 0);
					resonator.setGain(formant.gain, 0);
					resonator.update();
				}
			}

			void operator()(double** samples, double gain, int numChannels, int numSamples) noexcept
			{
				if (gain == 0.)
				{
					for (auto ch = 0; ch < numChannels; ++ch)
						SIMD::clear(samples[ch], numSamples);
					return;
				}

				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samples[ch];

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
					SIMD::multiply(smpls, gain, numSamples);
				}
			}
		private:
			ResonatorBank resonators;
		};

		struct Filter
		{
			Filter() :
				dualVowel(),
				voices(),
				gainDb(-120.),
				gain(1.)
			{
			}

			void prepare(double sampleRate) noexcept
			{
				dualVowel.prepare(sampleRate);
				for (auto& voice : voices)
					voice.prepare();
			}

			void operator()(const Params& params) noexcept
			{
				const auto _gainDb = params.gain;
				if (gainDb != _gainDb)
				{
					gainDb = _gainDb;
					if (gainDb <= -60.)
						gain = 0.;
					else
						gain = math::dbToAmp(gainDb);
				}

				if (dualVowel.wannaUpdate(params))
				{
					const auto& vowel = dualVowel.getVowel();
					for (auto& voice : voices)
						voice.updateResonators(vowel);
				}
			}

			// samples, gain, numChannels, numSamples, v
			void operator()(double** samples, int numChannels, int numSamples, int v) noexcept
			{
				voices[v](samples, gain, numChannels, numSamples);
			}
		private:
			DualVowel dualVowel;
			std::array<Voice, NumMPEChannels> voices;
			double gainDb, gain;
		};
	}
}