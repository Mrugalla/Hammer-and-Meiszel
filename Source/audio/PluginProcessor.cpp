#include "PluginProcessor.h"
#include "../arch/Math.h"
//#include "../HammerUndMeiszelTests.h"

namespace audio
{
	PluginProcessor::PluginProcessor(Params& _params, const arch::XenManager& _xen) :
		params(_params),
		xen(_xen),
		sampleRate(1.),
		autoMPE(),
		voiceSplit(),
		parallelProcessor(),
		modalFilter(xen),
		combFilter(xen)
	{
		//test::SpeedTestPB([&](double** samples, dsp::MidiBuffer& midi, int numChannels, int numSamples)
		//{
		//	prepare(44100.);
		//	operator()(samples, midi, 2, dsp::BlockSize2x);
		//}, 10, "i am speed");
	}

	void PluginProcessor::prepare(double _sampleRate)
	{
		sampleRate = _sampleRate;
		modalFilter.prepare(sampleRate);
		combFilter.prepare(sampleRate);
	}

	void PluginProcessor::operator()(double** samples, dsp::MidiBuffer& midi, int numChannels, int numSamples) noexcept
	{
		autoMPE(midi); voiceSplit(midi);

		const auto samplesInput = const_cast<const double**>(samples);

		const auto& modalMixParam = params(PID::ModalMix);
		const auto modalMix = static_cast<double>(modalMixParam.getValMod());

		const auto& modalHarmonizeParam = params(PID::ModalHarmonize);
		const auto modalHarmonize = static_cast<double>(modalHarmonizeParam.getValMod());

		const auto& modalSaturateParam = params(PID::ModalSaturate);
		const auto modalSaturate = static_cast<double>(modalSaturateParam.getValModDenorm());

		const auto& resoParam = params(PID::Resonance);
		const auto reso = static_cast<double>(resoParam.getValMod());

		const auto& combOctParam = params(PID::CombOct);
		const auto combOct = std::round(static_cast<double>(combOctParam.getValModDenorm()));
		const auto combSemi = combOct * xen.getXen();

		const auto& combFeedbackParam = params(PID::CombFeedback);
		auto combFeedback = static_cast<double>(combFeedbackParam.getValMod());
		if (combOct > 0.)
			combFeedback *= -1.;

		modalFilter.updateParameters(modalMix, modalHarmonize, modalSaturate, reso);
		combFilter.synthesizeWHead(numSamples);
		combFilter.updateParameters(combFeedback);

		for (auto v = 0; v < dsp::NumMPEChannels; ++v)
		{
			const auto& midiVoice = voiceSplit[v + 2];
			auto bandVoice = parallelProcessor[v];
			double* samplesVoice[] = { bandVoice.l, bandVoice.r };
			bool sleepy = parallelProcessor.isSleepy(v);
			
			if (!midiVoice.isEmpty() || !sleepy)
			{
				modalFilter(samplesInput, samplesVoice, midiVoice, numChannels, numSamples, v);
				combFilter(samplesVoice, midiVoice, combSemi, numChannels, numSamples, v);

				modalFilter.voices[v].detectSleepy(sleepy, samplesVoice, numChannels, numSamples);
				parallelProcessor.setSleepy(sleepy, v);
			}
		}

		parallelProcessor.joinReplace(samples, numChannels, numSamples);
	}

	void PluginProcessor::processBlockBypassed(double**, dsp::MidiBuffer&, int, int) noexcept
	{}

	void PluginProcessor::savePatch()
	{}

	void PluginProcessor::loadPatch()
	{}
}