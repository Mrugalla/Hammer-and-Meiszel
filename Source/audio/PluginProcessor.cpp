#include "PluginProcessor.h"
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
		flanger(xen),
		editorExists(false)
	{
		//test::SpeedTestPB([&](double** samples, dsp::MidiBuffer& midi, int numChannels, int numSamples)
		//{
		//	prepare(44100.);
		//	operator()(samples, midi, 2, dsp::BlockSize2x);
		//}, 10, "i am speed");

		startTimerHz(2);
	}

	void PluginProcessor::prepare(double _sampleRate)
	{
		sampleRate = _sampleRate;
		modalFilter.prepare(sampleRate);
		flanger.prepare(sampleRate);
	}

	void PluginProcessor::operator()(double** samples, dsp::MidiBuffer& midi, int numChannels, int numSamples) noexcept
	{
		autoMPE(midi); voiceSplit(midi);

		const auto& modalOctParam = params(PID::ModalOct);
		const auto& modalSemiParam = params(PID::ModalSemi);
		auto modalSemi = static_cast<double>(std::round(modalSemiParam.getValModDenorm()));
		modalSemi += static_cast<double>(std::round(modalOctParam.getValModDenorm())) * xen.getXen();

		const auto& modalMixParam = params(PID::ModalMix);
		const auto modalMix = static_cast<double>(modalMixParam.getValMod());

		const auto& modalHarmonizeParam = params(PID::ModalHarmonie);
		const auto modalHarmonize = static_cast<double>(modalHarmonizeParam.getValMod());

		const auto& modalSaturateParam = params(PID::ModalTonalitaet);
		const auto modalSaturate = static_cast<double>(modalSaturateParam.getValModDenorm());

		const auto& modalResoParam = params(PID::ModalResonanz);
		const auto modalReso = static_cast<double>(modalResoParam.getValMod());

		const auto& combOctParam = params(PID::CombOct);
		const auto combOct = std::round(static_cast<double>(combOctParam.getValModDenorm()));

		const auto& combSemiParam = params(PID::CombSemi);
		auto combSemi = static_cast<double>(std::round(combSemiParam.getValModDenorm()));
		combSemi += combOct * xen.getXen();

		const auto& combFeedbackParam = params(PID::CombRueckkopplung);
		auto combFeedback = static_cast<double>(combFeedbackParam.getValMod());
		if (combSemi > 0.)
			combFeedback *= -1.;

		const auto& combAPResonanzParam = params(PID::CombAPResonanz);
		const auto combAPResonanz = static_cast<double>(combAPResonanzParam.getValModDenorm());

		const auto& combAPShapeParam = params(PID::CombAPShape);
		const auto combAPShape = static_cast<double>(combAPShapeParam.getValMod());

		const auto& voiceAttaeckParam = params(PID::VoiceAttack);
		const auto voiceAttack = static_cast<double>(voiceAttaeckParam.getValModDenorm());
		const auto& voiceDecayParam = params(PID::VoiceDecay);
		const auto voiceDecay = static_cast<double>(voiceDecayParam.getValModDenorm());
		const auto& voiceSustainParam = params(PID::VoiceSustain);
		const auto voiceSustain = static_cast<double>(voiceSustainParam.getValMod());
		const auto& voiceReleaseParam = params(PID::VoiceRelease);
		const auto voiceRelease = static_cast<double>(voiceReleaseParam.getValModDenorm());

		modalFilter.updateParameters(modalMix, modalHarmonize, modalSaturate, modalReso);
		flanger.synthesizeWHead(numSamples);
		flanger.updateParameters(combFeedback, combAPShape, numChannels);
		
		const auto samplesInput = const_cast<const double**>(samples);

		for (auto v = 0; v < dsp::NumMPEChannels; ++v)
		{
			const auto& midiVoice = voiceSplit[v + 2];
			auto bandVoice = parallelProcessor[v];
			double* samplesVoice[] = { bandVoice.l, bandVoice.r };
			bool sleepy = parallelProcessor.isSleepy(v);
			
			if (!midiVoice.isEmpty() || !sleepy)
			{
				modalFilter.voices[v].updateParameters(voiceAttack, voiceDecay, voiceSustain, voiceRelease);
				modalFilter(samplesInput, samplesVoice, midiVoice, modalSemi, numChannels, numSamples, v);
				flanger(samplesVoice, midiVoice, combSemi, combAPResonanz, numChannels, numSamples, v);
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

	void PluginProcessor::timerCallback()
	{
		const auto noEditor = !editorExists.load();
		if (noEditor)
		{
			for (auto i = 0; i < 2; ++i)
			{
				auto& status = modalFilter.material.materials[i].status;
				status.store(dsp::modal::Status::Processing);
			}
		}
		
	}
}