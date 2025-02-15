#pragma once
#include "../../../../arch/XenManager.h"
#include "../../Resonator.h"
#include "Axiom.h"
#include "../../PRM.h"

namespace dsp
{
	namespace formant
	{
		struct Params
		{
			Params(double _vowelPos, double _q, double _gainDb,
				double _vowelPosMod, double _qMod, double _gainMod,
				int _vowelA, int _vowelB) :
				vowelPos(_vowelPos), q(_q), gain(_gainDb),
				vowelPosMod(_vowelPosMod), qMod(_qMod), gainMod(_gainMod),
				vowelA(_vowelA), vowelB(_vowelB)
			{
			}

			double vowelPos, q, gain, vowelPosMod, qMod, gainMod;
			int vowelA, vowelB;
		};

		// https://www.classes.cs.uchicago.edu/archive/1999/spring/CS295/Computing_Resources/Csound/CsManual3.48b1.HTML/Appendices/table3.html

		struct DualVowel
		{
			DualVowel() :
				a(toVowel(VowelClass::SopranoA)),
				b(toVowel(VowelClass::SopranoE)),
				params(-1., -1., -120., 0., 0., 0., -1, -1),
				updatedVowels(false)
			{
			}

			void prepare(double sampleRate) noexcept
			{
				a.prepare(sampleRate);
				b.prepare(sampleRate);
				ab.prepare(sampleRate);
				params = Params(-1., -1., -120., 0., 0., 0., -1, -1);
				updatedVowels = false;
			}

			void updateVowels(const Params& _params) noexcept
			{
				if (params.vowelA != _params.vowelA)
				{
					params.vowelA = _params.vowelA;
					const auto vowelClass = static_cast<VowelClass>(params.vowelA);
					a = toVowel(vowelClass);
					updatedVowels = true;
				}

				if (params.vowelB != _params.vowelB)
				{
					params.vowelB = _params.vowelB;
					const auto vowelClass = static_cast<VowelClass>(params.vowelB);
					b = toVowel(vowelClass);
					updatedVowels = true;
				}

				updatedVowels = false;
			}

			bool wannaUpdate(const Params& _params, double envGenMod) noexcept
			{
				bool update = updatedVowels;

				const auto vowelPos = math::limit(0., 1., _params.vowelPos + _params.vowelPosMod * envGenMod);
				const auto q = math::limit(0., 1., _params.q + _params.qMod * envGenMod);

				if (params.vowelPos != vowelPos ||
					params.q != q)
				{
					params.vowelPos = vowelPos;
					params.q = q;
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
			bool updatedVowels;
		};

		struct Voice
		{
			using ResonatorBank = std::array<ResonatorStereo2, NumFormants>;

			Voice() :
				resonators(),
				gainPRM(0.),
				sleepyTimer(0), sleepyThreshold(0),
				noteOn(false), sleepy(false)
			{
			}

			void prepare(double sampleRate) noexcept
			{
				for (auto& resonator : resonators)
					resonator.reset();
				gainPRM.prepare(sampleRate, 7.);
				sleepyTimer = 0;
				sleepyThreshold = static_cast<int>(sampleRate / 6.);
				noteOn = false;
				sleepy = false;
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

			void operator()(double** samples, const MidiBuffer& midi, const Params& params,
				const double* envGenMod, int numChannels, int numSamples) noexcept
			{
				auto s = 0;
				for (const auto& it : midi)
				{
					const auto msg = it.getMessage();
					const auto ts = it.samplePosition;
					if (msg.isNoteOn())
					{
						process(samples, params, envGenMod[s], numChannels, s, ts);
						noteOn = true;
						sleepy = false;
					}
					else if (msg.isNoteOff())
					{
						if (noteOn)
						{
							process(samples, params, envGenMod[s], numChannels, s, ts);
							noteOn = false;
						}
					}
					s = ts;
				}
				process(samples, params, envGenMod[s], numChannels, s, numSamples - s);
			}
		private:
			ResonatorBank resonators;
			PRMD gainPRM;

			int sleepyTimer, sleepyThreshold;
			bool noteOn, sleepy;

			void updateGain(const Params& params, double envGenMod, int start, int end) noexcept
			{
				const auto gainDb = params.gain + params.gainMod * envGenMod;
				const auto gain = math::dbToAmp(gainDb, -60.);
				auto gainInfo = gainPRM(gain, start, end);
				gainInfo.copyToBuffer(start, end);
			}

			void resonate(double** samples, int numChannels, int start, int end) noexcept
			{
				if (sleepy)
					return;
				const auto numSamples = end - start;
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samples[ch];

					for (auto s = start; s < end; ++s)
					{
						const auto dry = smpls[s];
						auto y = resonators[0](dry, ch);

						for (auto i = 1; i < NumFormants; ++i)
						{
							auto& resonator = resonators[i];
							y += resonator(dry, ch);
						}

						smpls[s] = y;

						if (!noteOn)
						{
							const auto eps = 1e-6;
							if (y * y > eps)
								sleepyTimer = 0;
							else
							{
								sleepyTimer += numSamples;
								if (sleepyTimer > sleepyThreshold)
								{
									sleepyTimer = 0;
									sleepy = true;
									break;
								}
							}
						}
					}

					SIMD::multiply(smpls, &gainPRM.buf[start], end - start);
				}
			}

			void process(double** samples, const Params& params,
				double envGenMod, int numChannels, int start, int end) noexcept
			{
				updateGain(params, envGenMod, start, end);
				resonate(samples, numChannels, start, end);
			}
		};

		struct Filter
		{
			Filter() :
				dualVowel(),
				voices()
			{
			}

			void prepare(double sampleRate) noexcept
			{
				dualVowel.prepare(sampleRate);
				for (auto& voice : voices)
					voice.prepare(sampleRate);
			}

			void operator()(const Params& params) noexcept
			{
				dualVowel.updateVowels(params);
			}

			// samples, params, gain, envGenMod, numChannels, numSamples, v
			void operator()(double** samples, const MidiBuffer& midi,
				const Params& params, const double* envGenMod,
				int numChannels, int numSamples, int v) noexcept
			{
				if (dualVowel.wannaUpdate(params, envGenMod[0]))
				{
					const auto& vowel = dualVowel.getVowel();
					for (auto& voice : voices)
						voice.updateResonators(vowel);
				}

				voices[v](samples, midi, params, envGenMod, numChannels, numSamples);
			}
		private:
			DualVowel dualVowel;
			std::array<Voice, NumMPEChannels> voices;
		};
	}
}