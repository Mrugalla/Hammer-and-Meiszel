#include "PluginProcessor.h"
#include <juce_core/juce_core.h>
//#include "../HammerUndMeiszelTests.h"

namespace audio
{
	PluginProcessor::PluginProcessor(Params& _params, const arch::XenManager& _xen) :
		params(_params), xen(_xen), sampleRate(1.),
		keySelector(),
		autoMPE(), voiceSplit(), parallelProcessor(),
		envGensAmp(), envGensMod(),
		noiseSynth(),
		modalFilter(), combFilter(),
		editorExists(false),
		recording(-1),
		recSampleIndex(0)
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
		combFilter.prepare(sampleRate);
		envGensAmp.prepare(sampleRate);
		envGensMod.prepare(sampleRate);
	}

	void PluginProcessor::operator()(double** samples,
		dsp::MidiBuffer& midi, int numChannels, int numSamples,
		bool playing) noexcept
	{
		{
			const auto& envGenAmpAttackParam = params(PID::EnvGenAmpAttack);
			const auto envGenAmpAttack = static_cast<double>(envGenAmpAttackParam.getValModDenorm());
			const auto& envGenAmpDecayParam = params(PID::EnvGenAmpDecay);
			const auto envGenAmpDecay = static_cast<double>(envGenAmpDecayParam.getValModDenorm());
			const auto& envGenAmpSustainParam = params(PID::EnvGenAmpSustain);
			const auto envGenAmpSustain = static_cast<double>(envGenAmpSustainParam.getValModDenorm());
			const auto& envGenAmpReleaseParam = params(PID::EnvGenAmpRelease);
			const auto envGenAmpRelease = static_cast<double>(envGenAmpReleaseParam.getValModDenorm());

			const auto& envGenModAttackParam = params(PID::EnvGenModAttack);
			const auto envGenModAttack = static_cast<double>(envGenModAttackParam.getValModDenorm());
			const auto& envGenModDecayParam = params(PID::EnvGenModDecay);
			const auto envGenModDecay = static_cast<double>(envGenModDecayParam.getValModDenorm());
			const auto& envGenModSustainParam = params(PID::EnvGenModSustain);
			const auto envGenModSustain = static_cast<double>(envGenModSustainParam.getValModDenorm());
			const auto& envGenModReleaseParam = params(PID::EnvGenModRelease);
			const auto envGenModRelease = static_cast<double>(envGenModReleaseParam.getValModDenorm());

			envGensAmp.updateParameters({ envGenAmpAttack, envGenAmpDecay, envGenAmpSustain, envGenAmpRelease });
			envGensMod.updateParameters({ envGenModAttack, envGenModDecay, envGenModSustain, envGenModRelease });
		}

		const auto& noiseBlendParam = params(PID::NoiseBlend);
		const auto noiseBlend = noiseBlendParam.getValMod();
		noiseSynth(samples, noiseBlend, numChannels, numSamples);

		const auto recordingIndex = recording.load();
		if (recordingIndex != -1)
		{
			auto& material = modalFilter.getMaterial(recordingIndex);
			auto& recBuffer = material.buffer;
			const auto numChannelsInv = 1. / static_cast<double>(numChannels);
			const auto bufferSizeInv = 1.f / static_cast<float>(recBuffer.size());
			for (auto s = 0; s < numSamples; ++s)
			{
				auto mid = samples[0][s];
				for (auto ch = 1; ch < numChannels; ++ch)
					mid += samples[ch][s];
				mid *= numChannelsInv;
				const auto midF = static_cast<float>(mid);

				const auto sF = static_cast<float>(recSampleIndex);
				const auto sR = sF * bufferSizeInv;
				const auto window = .5f - .5f * std::cos(sR * static_cast<float>(dsp::Tau));

				recBuffer[recSampleIndex] = midF * window;
				++recSampleIndex;

				if (recSampleIndex >= recBuffer.size())
				{
					material.load();
					recSampleIndex = 0;
					recording.store(-1);
					break;
				}
			}
		}

		const auto& keySelectorEnabledParam = params(PID::KeySelectorEnabled);
		const auto keySelectorEnabled = keySelectorEnabledParam.getValMod() > .5f;
		keySelector(midi, xen, keySelectorEnabled, playing);

		autoMPE(midi); voiceSplit(midi);

		const auto& modalOctParam = params(PID::ModalOct);
		const auto& modalSemiParam = params(PID::ModalSemi);
		auto modalSemi = static_cast<double>(std::round(modalSemiParam.getValModDenorm()));
		modalSemi += static_cast<double>(std::round(modalOctParam.getValModDenorm())) * xen.getXen();

		const auto& modalKeytrackParam = params(PID::ModalKeytrack);
		const auto modalKeytrack = static_cast<double>(modalKeytrackParam.getValMod());
		const auto& modalKeytrackEnvParam = params(PID::ModalKeytrackEnv);
		const auto modalKeytrackEnv = static_cast<double>(modalKeytrackEnvParam.getValModDenorm());
		const auto& modalKeytrackBreiteParam = params(PID::ModalKeytrackBreite);
		const auto modalKeytrackBreite = static_cast<double>(modalKeytrackBreiteParam.getValModDenorm());

		const auto& modalBlendParam = params(PID::ModalBlend);
		const auto modalBlend = static_cast<double>(modalBlendParam.getValMod());
		const auto& modalBlendEnvParam = params(PID::ModalBlendEnv);
		const auto modalBlendEnv = static_cast<double>(modalBlendEnvParam.getValModDenorm());
		const auto& modalBlendBreiteParam = params(PID::ModalBlendBreite);
		const auto modalBlendBreite = static_cast<double>(modalBlendBreiteParam.getValModDenorm());

		const auto& modalSpreizungParam = params(PID::ModalSpreizung);
		const auto modalSpreizung = static_cast<double>(modalSpreizungParam.getValModDenorm());
		const auto& modalSpreizungEnvParam = params(PID::ModalSpreizungEnv);
		const auto modalSpreizungEnv = static_cast<double>(modalSpreizungEnvParam.getValModDenorm());
		const auto& modalSpreizungBreiteParam = params(PID::ModalSpreizungBreite);
		const auto modalSpreizungBreite = static_cast<double>(modalSpreizungBreiteParam.getValModDenorm());

		const auto& modalHarmonieParam = params(PID::ModalHarmonie);
		const auto modalHarmonie = static_cast<double>(modalHarmonieParam.getValMod());
		const auto& modalHarmonieEnvParam = params(PID::ModalHarmonieEnv);
		const auto modalHarmonieEnv = static_cast<double>(modalHarmonieEnvParam.getValModDenorm());
		const auto& modalHarmonieBreiteParam = params(PID::ModalHarmonieBreite);
		const auto modalHarmonieBreite = static_cast<double>(modalHarmonieBreiteParam.getValModDenorm());

		const auto& modalKraftParam = params(PID::ModalKraft);
		const auto modalKraft = static_cast<double>(modalKraftParam.getValModDenorm());
		const auto& modalKraftEnvParam = params(PID::ModalKraftEnv);
		const auto modalKraftEnv = static_cast<double>(modalKraftEnvParam.getValModDenorm());
		const auto& modalKraftBreiteParam = params(PID::ModalKraftBreite);
		const auto modalKraftBreite = static_cast<double>(modalKraftBreiteParam.getValModDenorm());

		const auto& modalResoParam = params(PID::ModalResonanz);
		const auto modalReso = static_cast<double>(modalResoParam.getValMod());
		const auto& modalResoEnvParam = params(PID::ModalResonanzEnv);
		const auto modalResoEnv = static_cast<double>(modalResoEnvParam.getValModDenorm());
		const auto& modalResoBreiteParam = params(PID::ModalResonanzBreite);
		const auto modalResoBreite = static_cast<double>(modalResoBreiteParam.getValModDenorm());

		const auto& modalResoDampParam = params(PID::ModalResoDamp);
		const auto modalResoDamp = static_cast<double>(modalResoDampParam.getValMod());
		const auto& modalResoDampEnvParam = params(PID::ModalResoDampEnv);
		const auto modalResoDampEnv = static_cast<double>(modalResoDampEnvParam.getValModDenorm());
		const auto& modalResoDampBreiteParam = params(PID::ModalResoDampBreite);
		const auto modalResoDampBreite = static_cast<double>(modalResoDampBreiteParam.getValModDenorm());

		const dsp::modal::Voice::Parameters modalVoiceParams
		(
			modalKeytrack, modalBlend, modalSpreizung, modalHarmonie, modalKraft, modalReso, modalResoDamp,
			modalKeytrackEnv, modalBlendEnv, modalSpreizungEnv, modalHarmonieEnv, modalKraftEnv, modalResoEnv, modalResoDampEnv,
			modalKeytrackBreite, modalBlendBreite, modalSpreizungBreite, modalHarmonieBreite, modalKraftBreite, modalResoBreite, modalResoDampBreite
		);

		modalFilter();
		
		const auto& combOctParam = params(PID::CombOct);
		const auto combOct = std::round(static_cast<double>(combOctParam.getValModDenorm()));

		const auto& combSemiParam = params(PID::CombSemi);
		auto combSemi = static_cast<double>(std::round(combSemiParam.getValModDenorm()));
		combSemi += combOct * xen.getXen();

		const auto& combUnisonParam = params(PID::CombUnison);
		const auto combUnison = static_cast<double>(combUnisonParam.getValMod());

		const auto& combFeedbackParam = params(PID::CombFeedback);
		const auto combFeedback = static_cast<double>(combFeedbackParam.getValModDenorm());

		const auto& combFeedbackEnvParam = params(PID::CombFeedbackEnv);
		const auto combFeedbackEnv = static_cast<double>(combFeedbackEnvParam.getValModDenorm());
		const auto& combFeedbackWidthParam = params(PID::CombFeedbackWidth);
		const auto combFeedbackWidth = static_cast<double>(combFeedbackWidthParam.getValModDenorm());
		
		combFilter(numSamples);
		
		const auto samplesInput = const_cast<const double**>(samples);
		for (auto v = 0; v < dsp::NumMPEChannels; ++v)
		{
			const auto& midiVoice = voiceSplit[v + 2];
			auto bandVoice = parallelProcessor[v];
			double* samplesVoice[] = { bandVoice.l, bandVoice.r };

			// synthesize and process amplitude envelope generator
			const auto envGenAmpInfo = envGensAmp(midiVoice, numSamples, v);
			if (envGenAmpInfo.active)
				for (auto ch = 0; ch < numChannels; ++ch)
					dsp::SIMD::multiply(samplesVoice[ch], samplesInput[ch], envGenAmpInfo.data, numSamples);
			else
				for (auto ch = 0; ch < numChannels; ++ch)
					dsp::SIMD::clear(samplesVoice[ch], numSamples);

			// synthsize modulation envelope generator
			const auto envGenModInfo = envGensMod(midiVoice, numSamples, v);
			const auto envGenModVal = envGenModInfo.active ? envGenModInfo[0] : 0.;

			bool active = envGenModInfo.active || modalFilter.isRinging(v);
			if (active)
				modalFilter
				(
					samplesVoice,
					midiVoice, xen,
					modalVoiceParams,
					modalSemi, envGenModVal,
					numChannels, numSamples,
					v
				);

			active = active || combFilter.isRinging(v);
			if(active)
				combFilter
				(
					samplesVoice, midiVoice, xen,
					combSemi, combUnison,
					combFeedback, combFeedbackEnv, combFeedbackWidth,
					envGenModVal,
					numChannels, numSamples,
					v
				);

			parallelProcessor.setSleepy(!active, v);
		}

		parallelProcessor.joinReplace(samples, numChannels, numSamples);
	}

	void PluginProcessor::processBlockBypassed(double**, dsp::MidiBuffer&, int, int) noexcept
	{}

	void PluginProcessor::savePatch(arch::State& state)
	{
		keySelector.savePatch(state);
		for(auto i = 0; i < 2; ++i)
		{
			const auto& material = modalFilter.getMaterial(i);
			material.savePatch(state, "mat" + juce::String(i));
		}
	}

	void PluginProcessor::loadPatch(const arch::State& state)
	{
		keySelector.loadPatch(state);
		for (auto i = 0; i < 2; ++i)
		{
			auto& material = modalFilter.getMaterial(i);
			material.loadPatch(state, "mat" + juce::String(i));
			for (auto k = 0; k < 1000000; ++k)
				if (material.status.load() == dsp::modal::StatusMat::Processing)
				{
					material.reportUpdate();
					return;
				}
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
			status.store(dsp::modal::StatusMat::Processing);
		}
	}
}