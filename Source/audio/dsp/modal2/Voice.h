#pragma once
#include "ResonatorBank.h"
#include "../EnvelopeGenerator.h"
#include "../PRM.h"

namespace dsp
{
	namespace modal2
	{
		struct Voice
		{
			enum kParam{ kBlend, kSpreizung, kHarmonie, kKraft, kReso, kNumParams };

			struct Parameter
			{
				Parameter(double _val = 0., double _breite = 0., double _env = 0.) :
					val(_val),
					breite(_breite),
					env(_env)
				{}

				double val, breite, env;
			};

			struct Parameters
			{
				Parameters(double _blend = -0., double _spreizung = 0., double _harmonie = 0., double _kraft = 0., double _reso = 0.,
					double _blendEnv = 0., double _spreizungEnv = 0., double _harmonieEnv = 0., double _kraftEnv = 0., double _resoEnv = 0.,
					double _blendBreite = 0., double _spreizungBreite = 0., double _harmonieBreite = 0., double _kraftBreite = 0., double _resoBreite = 0.) :
					params
					{
						Parameter(_blend, _blendBreite, _blendEnv),
						Parameter(_spreizung, _spreizungBreite, _spreizungEnv),
						Parameter(_harmonie, _harmonieBreite, _harmonieEnv),
						Parameter(_kraft, _kraftBreite, _kraftEnv),
						Parameter(_reso, _resoBreite, _resoEnv)
					}
				{}

				const Parameter& operator[](int i) const noexcept
				{
					return params[i];
				}

				std::array<Parameter, kNumParams> params;
			};

			class SmoothStereoParameter
			{
				struct Param
				{
					Param() :
						prm(0.),
						val(0.)
					{}

					void prepare(double sampleRate, double smoothLenMs) noexcept
					{
						prm.prepare(sampleRate, smoothLenMs);
						val = 0.;
					}

					void processL(const Parameter& p, double env,
						double min, double max) noexcept
					{
						const auto info = prm(p.val - p.breite);
						process(info.val, env, min, max);
					}

					void processR(const Parameter& p, double env,
						double min, double max) noexcept
					{
						const auto info = prm(p.val + p.breite);
						process(info.val, env, min, max);
					}

					PRMBlockD prm;
					double val;
				private:
					void process(double infoVal, double env, double min, double max) noexcept
					{
						const auto y = infoVal + env;
						val = math::limit(min, max, y);
					}
				};
			public:
				SmoothStereoParameter() :
					params()
				{}

				void prepare(double sampleRate, double smoothLenMs) noexcept
				{
					for(auto& param: params)
						param.prepare(sampleRate, smoothLenMs);
				}

				bool operator()(const Parameter& p,
					double env, double min, double max, int numChannels) noexcept
				{
					env = p.env * env;
					params[0].processL(p, env, min, max);
					if(numChannels == 2)
						params[1].processR(p, env, min, max);
					return params[0].prm.info.smoothing || params[1].prm.info.smoothing;
				}

				double operator[](int ch) const noexcept
				{
					return params[ch].val;
				}

				std::array<Param, 2> params;
			};

			Voice(const EnvelopeGenerator::Parameters& envGenParams) :
				env(envGenParams),
				envBuffer(),
				materialStereo(),
				parameters(),
				resonatorBank(),
				wantsMaterialUpdate(true)
			{}

			void prepare(double sampleRate) noexcept
			{
				env.prepare(sampleRate);
				resonatorBank.prepare(materialStereo, sampleRate);
				const auto smoothLenMs = 40.;
				for (auto i = 0; i < kNumParams; ++i)
					parameters[i].prepare(sampleRate, smoothLenMs);
			}

			void operator()(const DualMaterial& dualMaterial, const Parameters& _parameters,
				const double** samplesSrc, double** samplesDest,
				const MidiBuffer& midi, const arch::XenManager& xen,
				double transposeSemi,
				int numChannels, int numSamples) noexcept
			{
				synthesizeEnvelope(midi, numSamples);
				processEnvelope(samplesSrc, samplesDest, numChannels, numSamples);
				updateParameters(dualMaterial, _parameters, numChannels);
				resonatorBank(materialStereo, samplesDest, midi, xen, transposeSemi, numChannels, numSamples);
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
			EnvelopeGenerator env;
			std::array<double, BlockSize> envBuffer;
			MaterialDataStereo materialStereo;
			std::array<SmoothStereoParameter, kNumParams> parameters;
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
				const auto envGenValue = env.getEnvNoSustain();

				if(parameters[kReso](_parameters[kReso], envGenValue, 0., 1., numChannels))
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						const auto& reso = parameters[kReso][ch];
						resonatorBank.setReso(reso, ch);
					}

				const bool blendSmooth = parameters[kBlend](_parameters[kBlend], envGenValue, 0., 1., numChannels);
				const bool spreziSmooth = parameters[kSpreizung](_parameters[kSpreizung], envGenValue, SpreizungMin, SpreizungMax, numChannels);
				const bool harmonieSmooth = parameters[kHarmonie](_parameters[kHarmonie], envGenValue, 0., 1., numChannels);
				
				bool updatesMaterial = false;

				if (blendSmooth || spreziSmooth || harmonieSmooth || wantsMaterialUpdate)
				{
					updatesMaterial = true;
					const auto& mat0 = dualMaterial[0].peakInfos;
					const auto& mat1 = dualMaterial[1].peakInfos;
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto& material = materialStereo[ch];

						const auto blend = parameters[kBlend][ch];
						const auto spreizung = parameters[kSpreizung][ch];
						const auto sprezi = std::exp(spreizung);
						const auto harmonie = parameters[kHarmonie][ch];
						const auto harmi = math::tanhApprox(Pi * harmonie);
						
						for (auto i = 0; i < NumFilters; ++i)
						{
							const auto& filt0 = mat0[i];
							const auto& filt1 = mat1[i];

							// BLEND
							const auto mag0 = filt0.mag;
							const auto mag1 = filt1.mag;
							const auto magRange = mag1 - mag0;
							const auto mag = mag0 + blend * magRange;
							material[i].mag = mag;

							const auto ratio0 = filt0.ratio;
							const auto ratio1 = filt1.ratio;
							const auto ratioRange = ratio1 - ratio0;
							const auto ratio = ratio0 + blend * ratioRange;
							material[i].ratio = ratio;

							// SPREIZUNG
							const auto iF = static_cast<double>(i);
							material[i].ratio += iF * sprezi;

							// HARMONIE
							const auto r = material[i].ratio;
							//const auto rFloor = std::floor(r);
							//const auto rFrac = r - rFloor;
							//const auto rFracWeighted = math::tanhApprox(2. * Tau * rFrac - Tau) * .5 + .5;
							//const auto rWeighted = rFloor + rFrac + harmi * (rFracWeighted - rFrac);
							//material[i].ratio = rWeighted;
							const auto rTuned = std::round(r);
							material[i].ratio = r + harmi * (rTuned - r);
						}
					}
				}
				
				bool kraftSmooth = parameters[kKraft](_parameters[kKraft], envGenValue, -1., 1., numChannels);
				if (kraftSmooth || wantsMaterialUpdate)
				{
					updatesMaterial = true;
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto& material = materialStereo[ch];

						auto kraft = parameters[kKraft][ch];
						kraft = (math::tanhApprox(Pi * kraft) + 1.) * .5;

						if (kraft != 0.)
						{
							for (auto i = 0; i < NumFilters; ++i)
							{
								const auto x = material[i].mag;
								const auto y = kraft * x / ((1. - kraft) - x + 2. * kraft * x);
								material[i].mag = y;
							}
						}
					}
				}
				
				if (updatesMaterial)
				{
					resonatorBank.updateFreqRatios(materialStereo, numChannels);
					wantsMaterialUpdate = false;
				}
			}
		};
	}
}

/*

todo:

optimize kraft, weil der braucht kein update von den frequency ratios
harmonie approached zu spät harmonisch wertvollen bereich

wenn spreizung tief liegt und harmonie hoch kann spreizung-modulation sound explodieren lassen
	minimal frequenz anheben?
	parameter smoothing?

*/







/*
				const auto blendEnv = _parameters.blend.env * envGenValue;
				const auto spreizungEnv = _parameters.spreizung.env * envGenValue;
				const auto harmonieEnv = _parameters.harmonie.env * envGenValue;
				const auto kraftEnv = _parameters.kraft.env * envGenValue;
				const auto resoEnv = _parameters.reso.env * envGenValue;

				const auto blend = _parameters.blend.val + blendEnv;
				const auto spreizung = _parameters.spreizung.val + spreizungEnv;
				const auto harmonie = _parameters.harmonie.val + harmonieEnv;
				const auto kraft = _parameters.kraft.val + kraftEnv;
				const auto reso = _parameters.reso.val + resoEnv;

				const auto blendBreite = _parameters.blend.breite;
				const auto spreizungBreite = _parameters.spreizung.breite;
				const auto harmonieBreite = _parameters.harmonie.breite;
				const auto kraftBreite = _parameters.kraft.breite;
				const auto resoBreite = _parameters.reso.breite;

				parameters.reso = reso;
				if (resoBreite == 0.)
				{
					const auto resi = math::limit(0., 1., reso);
					const auto resoInfoL = resoPRM(resi, 0);
					for (auto ch = 0; ch < numChannels; ++ch)
						resonatorBank.setReso(resoInfoL, ch);
				}
				else
				{
					double resiVals[2] =
					{
						math::limit(0., 1., reso - resoBreite),
						math::limit(0., 1., reso + resoBreite)
					};
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						const auto resoInfo = resoPRM(resiVals[ch], ch);
						resonatorBank.setReso(resoInfo, ch);
					}
				}

				if(!wantsMaterialUpdate)
					if(parameters.blend.val == blend &&
						parameters.spreizung.val == spreizung &&
						parameters.harmonie.val == harmonie &&
						parameters.kraft.val == kraft &&
						parameters.blend.breite == blendBreite &&
						parameters.spreizung.breite == spreizungBreite &&
						parameters.harmonie.breite == harmonieBreite &&
						parameters.kraft.breite == kraftBreite)
						return;

				parameters.blend.val = blend;
				parameters.spreizung.val = spreizung;
				parameters.harmonie.val = harmonie;
				parameters.kraft.val = kraft;
				parameters.blend.breite = blendBreite;
				parameters.spreizung.breite = spreizungBreite;
				parameters.harmonie.breite = harmonieBreite;
				parameters.kraft.breite = kraftBreite;

				double blendVals[2], spreizVals[2], harmonieVals[2], kraftVals[2];

				if (blendBreite == 0.)
					blendVals[0] = blendVals[1] = math::limit(0., 1., blend);
				else
				{
					blendVals[0] = math::limit(0., 1., blend - blendBreite);
					blendVals[1] = math::limit(0., 1., blend + blendBreite);
				}
				for(auto ch = 0; ch < numChannels; ++ch)
					blendPRM(blendVals[ch], ch);

				if (spreizungBreite == 0.)
				{
					const auto sprezi = std::exp(math::limit(SpreizungMin, SpreizungMax, spreizung));
					spreizVals[0] = spreizVals[1] = sprezi;
				}
				else
				{
					const auto sprezi0 = std::exp(math::limit(SpreizungMin, SpreizungMax, spreizung - spreizungBreite));
					const auto sprezi1 = std::exp(math::limit(SpreizungMin, SpreizungMax, spreizung + spreizungBreite));
					spreizVals[0] = sprezi0;
					spreizVals[1] = sprezi1;
				}
				for (auto ch = 0; ch < numChannels; ++ch)
					spreizungPRM(spreizVals[ch], ch);

				if (harmonieBreite == 0.)
					harmonieVals[0] = harmonieVals[1] = math::limit(0., 1., harmonie);
				else
				{
					harmonieVals[0] = math::limit(0., 1., harmonie - harmonieBreite);
					harmonieVals[1] = math::limit(0., 1., harmonie + harmonieBreite);
				}
				for (auto ch = 0; ch < numChannels; ++ch)
					harmoniePRM(harmonieVals[ch], ch);

				{
					const auto& mat0 = dualMaterial[0].peakInfos;
					const auto& mat1 = dualMaterial[1].peakInfos;

					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto& material = materialStereo[ch];
						const auto blendVal = blendVals[ch];
						const auto harmonieVal = harmonieVals[ch];
						const auto spreizVal = spreizVals[ch];

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
							material[i].ratio *= spreizVal;

							// HARMONIE
							const auto r = material[i].ratio;
							const auto frac = std::floor(r) - r;
							material[i].ratio += harmonieVal * frac;
						}
					}
				}

				if (kraftBreite == 0.)
				{
					const auto kkb = (math::tanhApprox(4. * kraft) + 1.) * .5;
					kraftVals[0] = kraftVals[1] = math::limit(-1., 1., kkb);
				}
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
				*/