#pragma once
#include "../PRM.h"
#include "../../../arch/XenManager.h"
#include <juce_dsp/juce_dsp.h>

namespace dsp
{
	namespace hnm
	{
		namespace lp
		{
			struct Params
			{
				Params(double _damp = 0., double _dampEnv = 0., double _dampWidth = 0.) :
					damp(_damp), dampEnv(_dampEnv), dampWidth(_dampWidth)
				{
				}

				double damp, dampEnv, dampWidth;
			};

			struct Voice
			{
				struct Val
				{
					Val() :
						pitch(24.),
						damp(0.),
						freqHz(420.),
						xenInfo(),
						prm(freqHz),
						info()
					{
						info.val = freqHz;
					}

					void prepare(double sampleRate) noexcept
					{
						prm.prepare(sampleRate, 13.);
					}

					void updateDamp(const arch::XenManager& xen,
						const Params& params, double envGenMod) noexcept
					{
						const auto nDamp = params.damp + envGenMod * params.dampEnv;
						if (damp == nDamp)
							return;
						damp = nDamp;
						updateFreqHz(xen);
					}

					void updateXen(const arch::XenManager& xen) noexcept
					{
						if (xenInfo == xen.getInfo())
							return;
						xenInfo = xen.getInfo();
						updateFreqHz(xen);
					}

					void updatePitch(const arch::XenManager& xen, double note, int s, int ts) noexcept
					{
						pitch = note;
						updateFreqHz(xen);
						info = prm(freqHz, s, ts);
					}

					void updatePRM(int s, int ts) noexcept
					{
						info = prm(s, ts);
					}

					void updateFreqHz(const arch::XenManager& xen) noexcept
					{
						freqHz = math::limit(0., 20000., xen.noteToFreqHz(pitch + damp));
						prm.value = freqHz;
					}

					bool isSmoothing() const noexcept
					{
						return info.smoothing;
					}

					double operator[](int s) const noexcept
					{
						return info[s];
					}

				private:
					double pitch, damp, freqHz;
					arch::XenManager::Info xenInfo;
					PRMD prm;
					PRMInfoD info;
				};

				Voice() :
					lp(),
					val()
				{
					lp.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
					lp.snapToZero();
				}

				void prepare(double sampleRate) noexcept
				{
					juce::dsp::ProcessSpec spec;
					spec.sampleRate = sampleRate;
					spec.maximumBlockSize = BlockSize;
					spec.numChannels = 2;
					lp.prepare(spec);
					lp.reset();
					val.prepare(sampleRate);
				}

				void operator()(double** samples, const MidiBuffer& midi,
					const Params& params, const arch::XenManager& xen,
					double envGenMod, int numChannels, int numSamples) noexcept
				{
					val.updateXen(xen);
					val.updateDamp(xen, params, envGenMod);
					auto s = 0;
					for (const auto it : midi)
					{
						const auto ts = it.samplePosition;
						process(samples, numChannels, s, ts);
						const auto msg = it.getMessage();
						if (msg.isNoteOn())
							val.updatePitch(xen, static_cast<double>(msg.getNoteNumber()), s, ts);
						s = ts;
					}
					val.updatePRM(s, numSamples);
					process(samples, numChannels, s, numSamples);
				}

			private:
				juce::dsp::StateVariableTPTFilter<double> lp;
				Val val;

				void process(double** samples, int numChannels, int start, int end) noexcept
				{
					if (end - start == 0)
						return;
					if (val.isSmoothing())
					{
						for (auto s = start; s < end; ++s)
						{
							lp.setCutoffFrequency(val[s]);
							for (auto ch = 0; ch < numChannels; ++ch)
							{
								auto smpls = samples[ch];
								const auto smpl = smpls[s];
								smpls[s] = lp.processSample(ch, smpl);
							}
						}
						return;
					}

					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto smpls = samples[ch];
						for (auto s = start; s < end; ++s)
						{
							const auto smpl = smpls[s];
							smpls[s] = lp.processSample(ch, smpl);
						}
					}
				}
			};

			struct Filter
			{
				Filter() :
					voices()
				{
				}

				void prepare(double sampleRate) noexcept
				{
					for (auto& voice : voices)
						voice.prepare(sampleRate);
				}

				void operator()(double** samples, const MidiBuffer& midi,
					const Params& params, const arch::XenManager& xen,
					double envGenMod, int numChannels, int numSamples, int v) noexcept
				{
					voices[v](samples, midi, params, xen, envGenMod, numChannels, numSamples);
				}

			private:
				std::array<Voice, NumMPEChannels> voices;
			};
		}
	}
}