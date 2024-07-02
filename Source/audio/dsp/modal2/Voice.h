#pragma once
#include "ResonatorBank.h"
#include "../EnvelopeGenerator.h"

namespace dsp
{
	namespace modal2
	{
		struct Voice
		{
			struct Parameters
			{
				Parameters(double _blend = 0., double _spreizung = 0., double _harmonie = 0., double _kraft = 0.,
					double _blendEnv = 0., double _spreizungEnv = 0., double _harmonieEnv = 0., double _kraftEnv = 0.) :
					blend(_blend),
					spreizung(_spreizung),
					harmonie(_harmonie),
					kraft(_kraft),
					blendEnv(_blendEnv),
					spreizungEnv(_spreizungEnv),
					harmonieEnv(_harmonieEnv),
					kraftEnv(_kraftEnv)
				{}

				const bool operator!=(const Parameters& other) const noexcept
				{
					return blend != other.blend ||
						spreizung != other.spreizung ||
						harmonie != other.harmonie ||
						kraft != other.kraft;
				}

				double blend, spreizung, harmonie, kraft;
				double blendEnv, spreizungEnv, harmonieEnv, kraftEnv;
			};

			Voice(const EnvelopeGenerator::Parameters& envGenParams) :
				material(),
				parameters(),
				env(envGenParams),
				envBuffer(),
				resonatorBank()
			{}

			void prepare(double sampleRate) noexcept
			{
				env.prepare(sampleRate);
				resonatorBank.prepare(material, sampleRate);
			}

			void operator()(const DualMaterial& dualMaterial, const Parameters& _parameters,
				const double** samplesSrc, double** samplesDest,
				const MidiBuffer& midi, const arch::XenManager& xen,
				double reso, double transposeSemi,
				int numChannels, int numSamples) noexcept
			{
				synthesizeEnvelope(midi, numSamples);
				processEnvelope(samplesSrc, samplesDest, numChannels, numSamples);
				updateParameters(dualMaterial, _parameters);
				resonatorBank(material, samplesDest, midi, xen, reso, transposeSemi, numChannels, numSamples);
			}

			bool isSleepy(bool sleepy, double** samplesDest,
				int numChannels, int numSamples) noexcept
			{
				if (env.state != EnvelopeGenerator::State::Release)
					return false;

				const bool bufferTooSmall = numSamples != BlockSize;
				if (bufferTooSmall)
					return sleepy;

				sleepy = bufferSilent(samplesDest, numChannels, numSamples);
				if (!sleepy)
					return sleepy;

				resonatorBank.reset();
				for (auto ch = 0; ch < numChannels; ++ch)
					SIMD::clear(samplesDest[ch], numSamples);
				return sleepy;
			}

		private:
			MaterialData material;
			Parameters parameters;
			EnvelopeGenerator env;
			std::array<double, BlockSize> envBuffer;
			ResonatorBank resonatorBank;

			void synthesizeEnvelope(const MidiBuffer& midi, int numSamples) noexcept
			{
				auto s = 0;
				for (const auto it : midi)
				{
					const auto msg = it.getMessage();
					const auto ts = it.samplePosition;
					if (msg.isNoteOn())
					{
						s = synthesizeEnvelope(s, ts);
						env.noteOn = true;
					}
					else if (msg.isNoteOff() || msg.isAllNotesOff())
					{
						s = synthesizeEnvelope(s, ts);
						env.noteOn = false;
					}
					else
						s = synthesizeEnvelope(s, ts);
				}

				synthesizeEnvelope(s, numSamples);
			}

			int synthesizeEnvelope(int s, int ts) noexcept
			{
				while (s < ts)
				{
					envBuffer[s] = env();
					++s;
				}
				return s;
			}

			void processEnvelope(const double** samplesSrc, double** samplesDest,
				int numChannels, int numSamples) noexcept
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto smplsSrc = samplesSrc[ch];
					auto smplsDest = samplesDest[ch];
					SIMD::multiply(smplsDest, smplsSrc, envBuffer.data(), numSamples);
				}
			}

			bool bufferSilent(double* const* samplesDest, int numChannels, int numSamples) const noexcept
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samplesDest[ch];
					for (auto s = 0; s < numSamples; ++s)
					{
						auto smpl = std::abs(smpls[s]);
						if (smpl > 1e-4)
							return false;
					}
				}
				return true;
			}

			void updateParameters(const DualMaterial& dualMaterial, const Parameters& _parameters) noexcept
			{
				auto envGenValue = env.getEnvNoSustain();
				envGenValue *= envGenValue;
				const auto blendEnv = _parameters.blendEnv * envGenValue;
				const auto spreizungEnv = _parameters.spreizungEnv * envGenValue;
				const auto harmonieEnv = _parameters.harmonieEnv * envGenValue;
				const auto kraftEnv = _parameters.kraftEnv * envGenValue;

				const auto blend = math::limit(0., 1., _parameters.blend + blendEnv);
				const auto spreizung = math::limit(SpreizungMin, SpreizungMax, _parameters.spreizung + spreizungEnv);
				const auto harmonie = math::limit(0., 1., _parameters.harmonie + harmonieEnv);
				const auto kraft = math::limit(-1., 1., _parameters.kraft + kraftEnv);

				if(parameters.blend == blend &&
					parameters.spreizung == spreizung &&
					parameters.harmonie == harmonie &&
					parameters.kraft == kraft)
					return;
				
				parameters.blend = blend;
				parameters.spreizung = spreizung;
				parameters.harmonie = harmonie;
				parameters.kraft = kraft;

				if (blend == 0.)
					material.copy(dualMaterial[0].peakInfos);
				else if (blend == 1.)
					material.copy(dualMaterial[1].peakInfos);
				else
					for (auto i = 0; i < NumFilters; ++i)
					{
						const auto& mat0 = dualMaterial[0].peakInfos;
						const auto& mat1 = dualMaterial[1].peakInfos;
						const auto& filt0 = mat0[i];
						const auto& filt1 = mat1[i];

						const auto mag0 = filt0.mag;
						const auto mag1 = filt1.mag;
						const auto mag = mag0 + blend * (mag1 - mag0);
						material[i].mag = mag;

						const auto ratio0 = filt0.ratio;
						const auto ratio1 = filt1.ratio;
						const auto ratio = ratio0 + blend * (ratio1 - ratio0);
						material[i].ratio = ratio;
					}

				if (spreizung != 0.)
				{
					const auto sprezi = std::exp(spreizung);
					for (auto i = 0; i < NumFilters; ++i)
						material[i].ratio *= sprezi;
				}

				if (harmonie != 0.)
					for (auto i = 0; i < NumFilters; ++i)
					{
						const auto ratio = material[i].ratio;
						const auto frac = std::floor(ratio) - ratio;
						material[i].ratio += harmonie * frac;
					}

				if (kraft != 0.)
				{
					const auto p = (math::tanhApprox(4. * kraft) + 1.) * .5;
					for (auto i = 0; i < NumFilters; ++i)
					{
						const auto x = material[i].mag;
						const auto y = p * x / ((1. - p) - x + 2. * p * x);
						material[i].mag = y;
					}
				}

				resonatorBank.updateFreqRatios(material);
			}
		};
	}
}

/*

todo:

implement material parameter smoothing more

optimize kraft, weil der braucht kein update von den frequency ratios

*/