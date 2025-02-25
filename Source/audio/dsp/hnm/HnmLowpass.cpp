#include "HnmLowpass.h"

namespace dsp
{
	namespace hnm
	{
		namespace lp
		{
			// Params

			Params::Params(double _damp, double _dampEnv, double _dampWidth) :
				damp(_damp), dampEnv(_dampEnv), dampWidth(_dampWidth)
			{
			}

			// Voice::Val

			Voice::Val::Val() :
				pitch(24.),
				pitchbend(0.),
				damps{ 0., 0. },
				prms{ 420., 69. }
			{
			}

			void Voice::Val::prepare(double sampleRate) noexcept
			{
				for(auto& prm: prms)
					prm.prepare(sampleRate, 13.);
			}

			void Voice::Val::operator()(const arch::XenManager& xen,
				const Params& params, double envGenMod,
				int numChannels, int numSamples) noexcept
			{
				const auto dampEnv = params.dampEnv * envGenMod;
				double dampValues[2] =
				{
					params.damp - params.dampWidth + dampEnv,
					params.damp + params.dampWidth + dampEnv
				};
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto dampVal = dampValues[ch];
					if (damps[ch] != dampVal)
					{
						damps[ch] = dampVal;
						updateFreqHz(xen, ch);
					}
					prms[ch](numSamples);
				}
			}

			void Voice::Val::updateXen(const arch::XenManager& xen, int numChannels) noexcept
			{
				for (auto ch = 0; ch < numChannels; ++ch)
					updateFreqHz(xen, ch);
			}
			
			void Voice::Val::updatePitch(const arch::XenManager& xen, double note, int numChannels) noexcept
			{
				pitch = note;
				for (auto ch = 0; ch < numChannels; ++ch)
					updateFreqHz(xen, ch);
			}

			void Voice::Val::updatePitchbend(const arch::XenManager& xen, double pb, int numChannels) noexcept
			{
				pitchbend = pb;
				for (auto ch = 0; ch < numChannels; ++ch)
					updateFreqHz(xen, ch);
			}

			void Voice::Val::updateFreqHz(const arch::XenManager& xen, int ch) noexcept
			{
				const auto pbRange = xen.getPitchbendRange();
				const auto damp = damps[ch];
				const auto freqHz = math::limit(0., 20000., xen.noteToFreqHz(pitch + damp + pitchbend * pbRange));
				prms[ch].value = freqHz;
			}

			bool Voice::Val::isSmoothing(int ch) const noexcept
			{
				return prms[ch].smoothing;
			}

			double Voice::Val::operator()(int ch, int s) const noexcept
			{
				return prms[ch][s];
			}

			// Voice

			Voice::Voice() :
				updateCutoffFuncs
			{
				[&](double v, int ch) { lps[ch].setCutoffFrequency(v); },
				[](double, int) {}
			},
				lps(),
				val(),
				sleepy()
			{
				for (auto& lp : lps)
				{
					lp.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
					lp.setResonance(1. / std::sqrt(4.));
					lp.snapToZero();
				}
			}

			void Voice::prepare(double sampleRate) noexcept
			{
				juce::dsp::ProcessSpec spec;
				spec.sampleRate = sampleRate;
				spec.maximumBlockSize = BlockSize;
				spec.numChannels = 1;
				for (auto& lp : lps)
				{
					lp.prepare(spec);
					lp.reset();
				}
				val.prepare(sampleRate);
				sleepy.prepare(sampleRate);
			}

			void Voice::operator()(double** samples,
				const Params& params, const arch::XenManager& xen,
				double envGenMod, int numChannels, int numSamples) noexcept
			{
				val(xen, params, envGenMod, numChannels, numSamples);
				process(samples, numChannels, numSamples);
				sleepy(samples, numChannels, numSamples);
			}

			void Voice::triggerXen(const arch::XenManager& xen, int numChannels) noexcept
			{
				val.updateXen(xen, numChannels);
			}

			void Voice::triggerNoteOn(const arch::XenManager& xen, double noteNumber, int numChannels) noexcept
			{
				sleepy.triggerNoteOn();
				val.updatePitch(xen, noteNumber, numChannels);
			}

			void Voice::triggerNoteOff() noexcept
			{
				sleepy.triggerNoteOff();
			}

			void Voice::triggerPitchbend(const arch::XenManager& xen, double pb, int numChannels) noexcept
			{
				val.updatePitchbend(xen, pb, numChannels);
			}

			bool Voice::isRinging() const noexcept
			{
				return sleepy.isRinging();
			}

			void Voice::process(double** samples, int numChannels, int numSamples) noexcept
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto cutoffIdx = val.isSmoothing(ch) ? 0 : 1;
					const auto& cutoffFunc = updateCutoffFuncs[cutoffIdx];
					auto& lp = lps[ch];

					for (auto s = 0; s < numSamples; ++s)
					{
						cutoffFunc(val(ch, s), ch);
						auto smpls = samples[ch];
						const auto smpl = smpls[s];
						const auto y = lp.processSample(0, smpl);
						smpls[s] = y;
					}
				}
			}

			// Filter

			Filter::Filter() :
				voices()
			{
			}

			void Filter::prepare(double sampleRate) noexcept
			{
				for (auto& voice : voices)
					voice.prepare(sampleRate);
			}

			void Filter::operator()(double** samples,
				const Params& params, const arch::XenManager& xen,
				double envGenMod, int numChannels, int numSamples, int v) noexcept
			{
				voices[v](samples, params, xen, envGenMod, numChannels, numSamples);
			}

			void Filter::triggerXen(const arch::XenManager& xen, int numChannels) noexcept
			{
				for (auto& voice : voices)
					voice.triggerXen(xen, numChannels);
			}

			void Filter::triggerNoteOn(const arch::XenManager& xen, double noteNumber, int numChannels, int v) noexcept
			{
				voices[v].triggerNoteOn(xen, noteNumber, numChannels);
			}

			void Filter::triggerNoteOff(int v) noexcept
			{
				voices[v].triggerNoteOff();
			}

			void Filter::triggerPitchbend(const arch::XenManager& xen, double pitchbend, int numChannels, int v) noexcept
			{
				voices[v].triggerPitchbend(xen, pitchbend, numChannels);
			}

			bool Filter::isRinging(int v) const noexcept
			{
				return voices[v].isRinging();
			}
		}
	}
}