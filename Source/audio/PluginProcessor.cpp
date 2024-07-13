#include "PluginProcessor.h"
#include <juce_core/juce_core.h>
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
		modalFilter(),
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

		const auto& modalBlendParam = params(PID::ModalBlend);
		const auto modalBlend = static_cast<double>(modalBlendParam.getValMod());
		const auto& modalBlendEnvParam = params(PID::ModalBlendEnv);
		const auto modalBlendEnv = static_cast<double>(modalBlendEnvParam.getValModDenorm());

		const auto& modalSpreizungParam = params(PID::ModalSpreizung);
		const auto modalSpreizung = static_cast<double>(modalSpreizungParam.getValModDenorm());
		const auto& modalSpreizungEnvParam = params(PID::ModalSpreizungEnv);
		const auto modalSpreizungEnv = static_cast<double>(modalSpreizungEnvParam.getValModDenorm());

		const auto& modalHarmonieParam = params(PID::ModalHarmonie);
		const auto modalHarmonie = static_cast<double>(modalHarmonieParam.getValMod());
		const auto& modalHarmonieEnvParam = params(PID::ModalHarmonieEnv);
		const auto modalHarmonieEnv = static_cast<double>(modalHarmonieEnvParam.getValModDenorm());

		const auto& modalKraftParam = params(PID::ModalKraft);
		const auto modalKraft = static_cast<double>(modalKraftParam.getValModDenorm());
		const auto& modalKraftEnvParam = params(PID::ModalKraftEnv);
		const auto modalKraftEnv = static_cast<double>(modalKraftEnvParam.getValModDenorm());

		const auto& modalResoParam = params(PID::ModalResonanz);
		const auto modalReso = static_cast<double>(modalResoParam.getValMod());

		const auto& modalBlendBreiteParam = params(PID::ModalBlendBreite);
		const auto modalBlendBreite = static_cast<double>(modalBlendBreiteParam.getValModDenorm());

		const auto& modalSpreizungBreiteParam = params(PID::ModalSpreizungBreite);
		const auto modalSpreizungBreite = static_cast<double>(modalSpreizungBreiteParam.getValModDenorm());

		const auto& modalKraftBreiteParam = params(PID::ModalKraftBreite);
		const auto modalKraftBreite = static_cast<double>(modalKraftBreiteParam.getValModDenorm());

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
		const auto voiceSustain = static_cast<double>(voiceSustainParam.getValModDenorm());
		const auto& voiceReleaseParam = params(PID::VoiceRelease);
		const auto voiceRelease = static_cast<double>(voiceReleaseParam.getValModDenorm());

		modalFilter
		(
			{ voiceAttack, voiceDecay, voiceSustain, voiceRelease }
		);
		flanger.synthesizeWHead(numSamples);
		flanger.updateParameters
		(
			combFeedback, combAPShape, numChannels
		);
		
		const auto samplesInput = const_cast<const double**>(samples);

		for (auto v = 0; v < dsp::NumMPEChannels; ++v)
		{
			const auto& midiVoice = voiceSplit[v + 2];
			auto bandVoice = parallelProcessor[v];
			double* samplesVoice[] = { bandVoice.l, bandVoice.r };
			bool sleepy = parallelProcessor.isSleepy(v);
			const bool awake = !sleepy;
			const bool atLeastOneNewNote = !midiVoice.isEmpty();
			
			if (awake || atLeastOneNewNote)
			{
				modalFilter
				(
					samplesInput, samplesVoice, midiVoice, xen,
					{
						modalBlend, modalSpreizung, modalHarmonie, modalKraft,
						modalBlendEnv, modalSpreizungEnv, modalHarmonieEnv, modalKraftEnv,
						modalBlendBreite, modalSpreizungBreite, modalKraftBreite
					},
					modalReso, modalSemi,
					numChannels, numSamples,
					v
				);
				//flanger
				//(
				//	samplesVoice, midiVoice, combSemi,
				//	combAPResonanz,
				//	numChannels, numSamples,
				//	v
				//);
				sleepy = modalFilter.isSleepy(sleepy, samplesVoice, numChannels, numSamples, v);
				parallelProcessor.setSleepy(sleepy, v);
			}
		}

		parallelProcessor.joinReplace(samples, numChannels, numSamples);
	}

	void PluginProcessor::processBlockBypassed(double**, dsp::MidiBuffer&, int, int) noexcept
	{}

	void PluginProcessor::savePatch(arch::State& state)
	{
		for(auto i = 0; i < 2; ++i)
		{
			const auto& material = modalFilter.getMaterial(i);
			const auto matStr = "mat" + juce::String(i);
			for (auto j = 0; j < dsp::modal2::NumFilters; ++j)
			{
				const auto& peakInfo = material.peakInfos[j];
				const auto peakStr = matStr + "pk" + juce::String(j);
				state.set(peakStr + "mg", peakInfo.mag);
				state.set(peakStr + "rt", peakInfo.ratio);
			}
		}
	}

	void PluginProcessor::loadPatch(const arch::State& state)
	{
		for (auto i = 0; i < 2; ++i)
		{
			auto& material = modalFilter.getMaterial(i);
			const auto matStr = "mat" + juce::String(i);
			for (auto j = 0; j < dsp::modal2::NumFilters; ++j)
			{
				auto& peakInfo = material.peakInfos[j];
				const auto peakStr = matStr + "pk" + juce::String(j);
				const auto magVal = state.get(peakStr + "mg");
				if (magVal != nullptr)
					peakInfo.mag = static_cast<double>(*magVal);
				const auto ratioVal = state.get(peakStr + "rt");
				if (ratioVal != nullptr)
					peakInfo.ratio = static_cast<double>(*ratioVal);
			}
			material.status.store(dsp::modal2::Status::UpdatedMaterial);
		}
	}

	void PluginProcessor::timerCallback()
	{
		const auto edtrExists = !editorExists.load();
		if (edtrExists)
			return;
		for (auto i = 0; i < 2; ++i)
		{
			auto& material = modalFilter.getMaterial(i);
			auto& status = material.status;
			status.store(dsp::modal2::Status::Processing);
		}
	}
}