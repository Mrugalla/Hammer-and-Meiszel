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
				damp(0.),
				freqHz(420.),
				xenInfo(),
				prm(freqHz),
				info()
			{
				info.val = freqHz;
			}

			void Voice::Val::prepare(double sampleRate) noexcept
			{
				prm.prepare(sampleRate, 13.);
			}

			void Voice::Val::operator()(const arch::XenManager& xen,
				const Params& params, double envGenMod,
				int numSamples) noexcept
			{
				updateXen(xen);
				updateDamp(xen, params, envGenMod);
				info = prm(numSamples);
			}

			void Voice::Val::updateXen(const arch::XenManager& xen) noexcept
			{
				if (xenInfo == xen.getInfo())
					return;
				xenInfo = xen.getInfo();
				updateFreqHz(xen);
			}

			void Voice::Val::updateDamp(const arch::XenManager& xen,
				const Params& params, double envGenMod) noexcept
			{
				const auto nDamp = params.damp + envGenMod * params.dampEnv;
				if (damp == nDamp)
					return;
				damp = nDamp;
				updateFreqHz(xen);
			}
			void Voice::Val::updatePitch(const arch::XenManager& xen, double note) noexcept
			{
				pitch = note;
				updateFreqHz(xen);
			}

			void Voice::Val::updateFreqHz(const arch::XenManager& xen) noexcept
			{
				freqHz = math::limit(0., 20000., xen.noteToFreqHz(pitch + damp));
				prm.value = freqHz;
			}

			bool Voice::Val::isSmoothing() const noexcept
			{
				return info.smoothing;
			}

			double Voice::Val::operator[](int s) const noexcept
			{
				return info[s];
			}

			// Voice

			Voice::Voice() :
				updateCutoffFuncs
			{
				[&f = lp](double v) { f.setCutoffFrequency(v); },
				[](double) {}
			},
				lp(),
				val(),
				sleepy()
			{
				lp.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
				lp.snapToZero();
			}

			void Voice::prepare(double sampleRate) noexcept
			{
				juce::dsp::ProcessSpec spec;
				spec.sampleRate = sampleRate;
				spec.maximumBlockSize = BlockSize;
				spec.numChannels = 2;
				lp.prepare(spec);
				lp.reset();
				val.prepare(sampleRate);
				sleepy.prepare(sampleRate);
			}

			void Voice::operator()(double** samples,
				const Params& params, const arch::XenManager& xen,
				double envGenMod, int numChannels, int numSamples) noexcept
			{
				val(xen, params, envGenMod, numSamples);
				process(samples, numChannels, numSamples);
				sleepy(samples, numChannels, numSamples);
			}

			void Voice::triggerNoteOn(const arch::XenManager& xen, double noteNumber) noexcept
			{
				sleepy.triggerNoteOn();
				val.updatePitch(xen, noteNumber);
			}

			void Voice::triggerNoteOff() noexcept
			{
				sleepy.triggerNoteOff();
			}

			void Voice::triggerPitchbend(const arch::XenManager& xen, double noteNumber) noexcept
			{
				val.updatePitch(xen, noteNumber);
			}

			bool Voice::isRinging() const noexcept
			{
				return sleepy.isRinging();
			}

			void Voice::process(double** samples, int numChannels, int numSamples) noexcept
			{
				const auto cutoffIdx = val.isSmoothing() ? 0 : 1;
				const auto& cutoffFunc = updateCutoffFuncs[cutoffIdx];
				for (auto s = 0; s < numSamples; ++s)
				{
					cutoffFunc(val[s]);
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto smpls = samples[ch];
						const auto smpl = smpls[s];
						const auto y = lp.processSample(ch, smpl);
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

			void Filter::triggerNoteOn(const arch::XenManager& xen, double noteNumber, int v) noexcept
			{
				voices[v].triggerNoteOn(xen, noteNumber);
			}

			void Filter::triggerNoteOff(int v) noexcept
			{
				voices[v].triggerNoteOff();
			}

			void Filter::triggerPitchbend(const arch::XenManager& xen, double pitchbend, int v) noexcept
			{
				voices[v].triggerPitchbend(xen, pitchbend);
			}

			bool Filter::isRinging(int v) const noexcept
			{
				return voices[v].isRinging();
			}
		}
	}
}