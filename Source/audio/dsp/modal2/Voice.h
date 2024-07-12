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
				Parameters(double _blend = -420., double _spreizung = 0., double _harmonie = 0., double _kraft = 0.,
					double _blendEnv = 0., double _spreizungEnv = 0., double _harmonieEnv = 0., double _kraftEnv = 0.,
					double _blendBreite = 0., double _harmonieBreite = 0., double _kraftBreite = 0.) :
					blend(_blend),
					spreizung(_spreizung),
					harmonie(_harmonie),
					kraft(_kraft),
					blendEnv(_blendEnv),
					spreizungEnv(_spreizungEnv),
					harmonieEnv(_harmonieEnv),
					kraftEnv(_kraftEnv),
					blendBreite(_blendBreite),
					harmonieBreite(_harmonieBreite),
					kraftBreite(_kraftBreite)
				{}

				const bool operator!=(const Parameters& other) const noexcept
				{
					return blend != other.blend ||
						spreizung != other.spreizung ||
						harmonie != other.harmonie ||
						kraft != other.kraft ||
						blendBreite != other.blendBreite ||
						harmonieBreite != other.harmonieBreite ||
						kraftBreite != other.kraftBreite;
				}

				double blend, spreizung, harmonie, kraft;
				double blendEnv, spreizungEnv, harmonieEnv, kraftEnv;
				double blendBreite, harmonieBreite, kraftBreite;
			};

			Voice(const EnvelopeGenerator::Parameters& envGenParams) :
				materialStereo(),
				parameters(),
				env(envGenParams),
				envBuffer(),
				resonatorBank(),
				wantsMaterialUpdate(true)
			{}

			void prepare(double sampleRate) noexcept
			{
				env.prepare(sampleRate);
				resonatorBank.prepare(materialStereo, sampleRate);
			}

			void operator()(const DualMaterial& dualMaterial, const Parameters& _parameters,
				const double** samplesSrc, double** samplesDest,
				const MidiBuffer& midi, const arch::XenManager& xen,
				double reso, double transposeSemi,
				int numChannels, int numSamples) noexcept
			{
				synthesizeEnvelope(midi, numSamples);
				processEnvelope(samplesSrc, samplesDest, numChannels, numSamples);
				updateParameters(dualMaterial, _parameters, numChannels);
				resonatorBank(materialStereo, samplesDest, midi, xen, reso, transposeSemi, numChannels, numSamples);
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

			void reportMaterialUpdate() noexcept
			{
				wantsMaterialUpdate = true;
			}

		private:
			MaterialDataStereo materialStereo;
			Parameters parameters;
			EnvelopeGenerator env;
			std::array<double, BlockSize> envBuffer;
			ResonatorBank resonatorBank;
			bool wantsMaterialUpdate;

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

			void updateParameters(const DualMaterial& dualMaterial,
				const Parameters& _parameters, int numChannels) noexcept
			{
				auto envGenValue = env.getEnvNoSustain();
				envGenValue *= envGenValue;
				const auto blendEnv = _parameters.blendEnv * envGenValue;
				const auto spreizungEnv = _parameters.spreizungEnv * envGenValue;
				const auto harmonieEnv = _parameters.harmonieEnv * envGenValue;
				const auto kraftEnv = _parameters.kraftEnv * envGenValue;

				const auto blend = _parameters.blend + blendEnv;
				const auto spreizung = _parameters.spreizung + spreizungEnv;
				const auto harmonie = _parameters.harmonie + harmonieEnv;
				const auto kraft = _parameters.kraft + kraftEnv;

				const auto blendBreite = _parameters.blendBreite;
				const auto harmonieBreite = _parameters.harmonieBreite;
				const auto kraftBreite = _parameters.kraftBreite;

				if(!wantsMaterialUpdate)
					if(parameters.blend == blend &&
						parameters.spreizung == spreizung &&
						parameters.harmonie == harmonie &&
						parameters.kraft == kraft &&
						parameters.blendBreite == blendBreite &&
						parameters.harmonieBreite == harmonieBreite &&
						parameters.kraftBreite == kraftBreite)
						return;
				
				parameters.blend = blend;
				parameters.spreizung = spreizung;
				parameters.harmonie = harmonie;
				parameters.kraft = kraft;
				parameters.blendBreite = blendBreite;
				parameters.harmonieBreite = harmonieBreite;
				parameters.kraftBreite = kraftBreite;

				double blendVals[2], harmVals[2], kraftVals[2];

				if (blendBreite == 0.)
					blendVals[0] = blendVals[1] = math::limit(0., 1., blend);
				else
				{
					blendVals[0] = math::limit(0., 1., blend - blendBreite);
					blendVals[1] = math::limit(0., 1., blend + blendBreite);
				}

				const auto sprezi = math::limit(SpreizungMin, SpreizungMax, std::exp(spreizung));

				if (harmonieBreite == 0.)
					harmVals[0] = harmVals[1] = math::limit(0., 1., harmonie);
				else
				{
					harmVals[0] = math::limit(0., 1., harmonie - harmonieBreite);
					harmVals[1] = math::limit(0., 1., harmonie + harmonieBreite);
				}

				{
					const auto& mat0 = dualMaterial[0].peakInfos;
					const auto& mat1 = dualMaterial[1].peakInfos;

					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto& material = materialStereo[ch];
						const auto blendVal = blendVals[ch];
						const auto harmVal = harmVals[ch];

						for (auto i = 0; i < NumFilters; ++i)
						{
							const auto& filt0 = mat0[i];
							const auto& filt1 = mat1[i];

							// BLEND
							const auto mag0 = filt0.mag;
							const auto mag1 = filt1.mag;
							const auto magRange = mag1 - mag0;
							const auto mag = mag0 + blendVal * magRange;
							material[i].mag = mag;

							const auto ratio0 = filt0.ratio;
							const auto ratio1 = filt1.ratio;
							const auto ratioRange = ratio1 - ratio0;
							const auto ratio = ratio0 + blendVal * ratioRange;
							material[i].ratio = ratio;

							// SPREIZUNG
							material[i].ratio *= sprezi;

							// HARMONIE
							{
								const auto r = material[i].ratio;
								const auto frac = std::floor(r) - r;
								material[i].ratio += harmVal * frac;
							}
						}
					}
				}
				
				if (kraftBreite == 0.)
					kraftVals[0] = kraftVals[1] = math::limit(-1., 1., kraft);
				else
				{
					const auto kb0 = kraft - kraftBreite;
					const auto kb1 = kraft + kraftBreite;
					kraftVals[0] = math::limit(-1., 1., (math::tanhApprox(4. * kb0) + 1.) * .5);
					kraftVals[1] = math::limit(-1., 1., (math::tanhApprox(4. * kb1) + 1.) * .5);
				}

				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto kraftVal = kraftVals[ch];
					if (kraftVal != 0.)
					{
						auto& material = materialStereo[ch];
						for (auto i = 0; i < NumFilters; ++i)
						{
							const auto x = material[i].mag;
							const auto y = kraftVal * x / ((1. - kraftVal) - x + 2. * kraftVal * x);
							material[i].mag = y;
						}
					}
				}
				
				resonatorBank.updateFreqRatios(materialStereo, numChannels);
				wantsMaterialUpdate = false;
			}
		};
	}
}

/*

todo:

implement material parameter smoothing
optimize kraft, weil der braucht kein update von den frequency ratios
alle breite parameter genauer betrachten
	machen sie bereits das richtige?
mach spreizungbreite statt harmoniebreite
	weil wenn man harmonie will, will mans eh überall
	aber spreizung könnte unterschiedlich das licht verteilen

*/