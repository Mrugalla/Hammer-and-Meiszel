#include "PluginProcessor.h"
#include <juce_core/juce_core.h>

namespace audio
{
	PluginProcessor::PluginProcessor(Params& _params, arch::XenManager& _xen) :
		params(_params), xen(_xen), sampleRate(1.),
		keySelector(),
		autoMPE(), voiceSplit(), parallelProcessor(), formantLayer(),
		envGensAmp(), envGensMod(),
		noiseSynth(),
		modalFilter(), formantFilter(), combFilter(), lowpass(),
		editorExists(false),
		recording(-1),
		recSampleIndex(0)
	{
		startTimerHz(2);

		xen.updateFunc = [&](const arch::XenManager::Info&, int numChannels)
		{
			modalFilter.triggerXen(xen, numChannels);
			combFilter.triggerXen(xen, numChannels);
			lowpass.triggerXen(xen, numChannels);
		};
	}

	void PluginProcessor::prepare(double _sampleRate)
	{
		sampleRate = _sampleRate;
		keySelector.prepare();
		modalFilter.prepare(sampleRate);
		formantFilter.prepare(sampleRate);
		combFilter.prepare(sampleRate);
		envGensAmp.prepare(sampleRate);
		envGensMod.prepare(sampleRate);
		lowpass.prepare(sampleRate);
	}

	void PluginProcessor::operator()(double** samples,
		dsp::MidiBuffer& midi, int numChannels, int numSamples,
		bool playing) noexcept
	{
#if PPDTestEnvelope
		const auto& envGenAmpAttackParam = params(PID::EnvGenAmpAttack);
		const auto envGenAmpAttack = static_cast<double>(envGenAmpAttackParam.getValModDenorm());
		const auto& envGenAmpDecayParam = params(PID::EnvGenAmpDecay);
		const auto envGenAmpDecay = static_cast<double>(envGenAmpDecayParam.getValModDenorm());
		const auto& envGenAmpSustainParam = params(PID::EnvGenAmpSustain);
		const auto envGenAmpSustain = static_cast<double>(envGenAmpSustainParam.getValModDenorm());
		const auto& envGenAmpReleaseParam = params(PID::EnvGenAmpRelease);
		const auto envGenAmpRelease = static_cast<double>(envGenAmpReleaseParam.getValModDenorm());
		envGensAmp.updateParameters({ envGenAmpAttack, envGenAmpDecay, envGenAmpSustain, envGenAmpRelease });

		const auto envGenAmpInfo = envGensAmp(midi, numSamples, 0);
		if (envGenAmpInfo.active)
			for (auto ch = 0; ch < numChannels; ++ch)
				dsp::SIMD::copy(samples[ch], envGenAmpInfo.data, numSamples);
		else
			for (auto ch = 0; ch < numChannels; ++ch)
				dsp::SIMD::clear(samples[ch], numSamples);
#else
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
					s = numSamples;
				}
			}
		}

		const auto& keySelectorEnabledParam = params(PID::KeySelectorEnabled);
		const auto keySelectorEnabled = keySelectorEnabledParam.getValMod() > .5f;
		keySelector(midi, xen, keySelectorEnabled, playing);

		autoMPE(midi); voiceSplit(midi, numSamples);

		const auto& modalOctParam = params(PID::ModalOct);
		const auto& modalSemiParam = params(PID::ModalSemi);
		auto modalSemi = static_cast<double>(std::round(modalSemiParam.getValModDenorm()));
		modalSemi += static_cast<double>(std::round(modalOctParam.getValModDenorm())) * xen.getXen();

		modalFilter.setTranspose(xen, modalSemi, numChannels);

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

		const dsp::modal::Voice::Parameters modalParams
		(
			modalBlend, modalSpreizung, modalHarmonie, modalKraft, modalReso,
			modalBlendEnv, modalSpreizungEnv, modalHarmonieEnv, modalKraftEnv, modalResoEnv,
			modalBlendBreite, modalSpreizungBreite, modalHarmonieBreite, modalKraftBreite, modalResoBreite
		);

		modalFilter();

		const auto& formantPosParam = params(PID::FormantPos);
		const auto formantPos = static_cast<double>(formantPosParam.getValMod());
		const auto& formantPosEnvParam = params(PID::FormantPosEnv);
		const auto formantPosEnv = static_cast<double>(formantPosEnvParam.getValModDenorm());
		const auto& formantPosWidthParam = params(PID::FormantPosWidth);
		const auto formantPosWidth = static_cast<double>(formantPosWidthParam.getValModDenorm());

		const auto& formantQParam = params(PID::FormantQ);
		const auto formantQ = static_cast<double>(formantQParam.getValMod());
		const auto& formantQEnvParam = params(PID::FormantQEnv);
		const auto formantQEnv = static_cast<double>(formantQEnvParam.getValModDenorm());
		const auto& formantQWidthParam = params(PID::FormantQWidth);
		const auto formantQWidth = static_cast<double>(formantQWidthParam.getValModDenorm());

		const auto& formantAParam = params(PID::FormantA);
		const auto formantVowelA = static_cast<int>(std::round(formantAParam.getValModDenorm()));
		const auto& formantBParam = params(PID::FormantB);
		const auto formantVowelB = static_cast<int>(std::round(formantBParam.getValModDenorm()));

		dsp::formant::Params formantParams
		(
			formantPos, formantQ,
			formantPosEnv, formantQEnv,
			formantPosWidth, formantQWidth,
			formantVowelA, formantVowelB
		);

		const auto& formantDecayParam = params(PID::FormantDecay);
		const auto formantDecay = static_cast<double>(formantDecayParam.getValModDenorm());
		const auto& formantGainDbParam = params(PID::FormantGain);
		const auto formantGainDb = static_cast<double>(formantGainDbParam.getValModDenorm());

		formantFilter.updateEnvelopes(formantDecay, envGenAmpRelease, formantGainDb, numSamples);

		const auto edo = xen.getXen();

		const auto& combOctParam = params(PID::CombOct);
		const auto combOct = std::round(static_cast<double>(combOctParam.getValModDenorm()));
		const auto& combSemiParam = params(PID::CombSemi);
		auto combSemi = static_cast<double>(std::round(combSemiParam.getValModDenorm()));
		combSemi += combOct * edo;

		const auto& combUnisonParam = params(PID::CombUnison);
		const auto combUnison = static_cast<double>(combUnisonParam.getValMod());

		const auto& combFeedbackParam = params(PID::CombFeedback);
		const auto combFeedback = static_cast<double>(combFeedbackParam.getValModDenorm());
		const auto& combFeedbackEnvParam = params(PID::CombFeedbackEnv);
		const auto combFeedbackEnv = static_cast<double>(combFeedbackEnvParam.getValModDenorm());
		const auto& combFeedbackWidthParam = params(PID::CombFeedbackWidth);
		const auto combFeedbackWidth = static_cast<double>(combFeedbackWidthParam.getValModDenorm());

		const dsp::hnm::Params combParams
		(
			combSemi, combUnison, combFeedback, combFeedbackEnv, combFeedbackWidth
		);

		const auto& dampParam = params(PID::Damp);
		const auto damp = static_cast<double>(dampParam.getValModDenorm()) * edo;
		const auto& dampEnvParam = params(PID::DampEnv);
		const auto dampEnv = static_cast<double>(dampEnvParam.getValModDenorm()) * edo;
		const auto& dampWidthParam = params(PID::DampWidth);
		const auto dampWidth = static_cast<double>(dampWidthParam.getValModDenorm()) * edo;

		const dsp::hnm::lp::Params lpParams(damp, dampEnv, dampWidth);

		const auto samplesInput = const_cast<const double**>(samples);

		for (auto v = 0; v < dsp::NumMPEChannels; ++v)
		{
			const auto& midiVoice = voiceSplit[v + 2];
			auto bandVoice = parallelProcessor[v];

			auto start = 0;
			for(const auto it: midiVoice)
			{
				const auto msg = it.getMessage();
				const auto end = it.samplePosition;
				const auto numSamplesEvt = end - start;

				const double* samplesInputEvt[] = { &samplesInput[0][start], &samplesInput[1][start] };
				double* samplesVoiceEvt[] = { &bandVoice.l[start], &bandVoice.r[start] };

				const auto envGenAmpActive = envGensAmp.processGain
				(
					samplesVoiceEvt, samplesInputEvt,
					numChannels, numSamplesEvt, v
				);

				// synthesize the modulation envelope generator
				const auto envGenModInfo = envGensMod(v, numSamplesEvt);
				const auto envGenModVal = envGenModInfo.active ? envGenModInfo[0] : 0.;

				const bool modalRinging = modalFilter.isRinging(v);
				const bool formantsRinging = formantFilter.isRinging(v);
				bool active = envGenAmpActive || modalRinging || formantsRinging;
				if (active)
				{
					double* layerVoiceEvt[] = { formantLayer[0].data(), formantLayer[1].data() };
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						const auto smplsVoice = samplesVoiceEvt[ch];
						auto layer = layerVoiceEvt[ch];
						dsp::SIMD::copy(layer, smplsVoice, numSamplesEvt);
					}
					
					modalFilter
					(
						samplesVoiceEvt,
						modalParams,
						envGenModVal,
						numChannels,
						numSamplesEvt,
						v
					);


					formantFilter
					(
						layerVoiceEvt,
						formantParams,
						envGenModVal,
						numChannels,
						numSamplesEvt,
						v
					);

					for (auto ch = 0; ch < numChannels; ++ch)
						dsp::SIMD::add(samplesVoiceEvt[ch], layerVoiceEvt[ch], numSamplesEvt);
				}
				
				const bool combRinging = combFilter.isRinging(v);
				active = active || combRinging;
				if (active)
					combFilter
					(
						samplesVoiceEvt, xen,
						combParams, envGenModVal,
						numChannels, numSamplesEvt, v
					);
				active = active || lowpass.isRinging(v);
				if (active)
					lowpass
					(
						samplesVoiceEvt,
						lpParams, xen,
						envGenModVal,
						numChannels, numSamplesEvt,
						v
					);

				parallelProcessor.setSleepy(!active, v);
				start = end;

				if (msg.isNoteOn())
				{
					envGensAmp.triggerNoteOn(true, v);
					envGensMod.triggerNoteOn(true, v);
					const auto noteNumber = static_cast<double>(msg.getNoteNumber());
					modalFilter.triggerNoteOn(xen, noteNumber, numChannels, v);
					formantFilter.triggerNoteOn(v);
					combFilter.triggerNoteOn(xen, noteNumber, numChannels, v);
					lowpass.triggerNoteOn(xen, noteNumber, numChannels, v);
				}
				else if (msg.isNoteOff())
				{
					envGensAmp.triggerNoteOn(false, v);
					envGensMod.triggerNoteOn(false, v);
					modalFilter.triggerNoteOff(v);
					formantFilter.triggerNoteOff(v);
					combFilter.triggerNoteOff(v);
					lowpass.triggerNoteOff(v);
				}
				else if (msg.isAllNotesOff())
				{
					envGensAmp.triggerNoteOn(false, v);
					envGensMod.triggerNoteOn(false, v);
					modalFilter.triggerNoteOff(v);
					formantFilter.triggerNoteOff(v);
					combFilter.triggerNoteOff(v);
					lowpass.triggerNoteOff(v);
				}
				else if (msg.isPitchWheel())
				{
					static constexpr auto PB = static_cast<double>(0x3fff);
					static constexpr auto PBInv = 1. / PB;
					const auto pb = static_cast<double>(2 * msg.getPitchWheelValue()) * PBInv - 1;
					modalFilter.triggerPitchbend(xen, pb, numChannels, v);
					combFilter.triggerPitchbend(xen, pb, numChannels, v);
					lowpass.triggerPitchbend(xen, pb, numChannels, v);
				}
			}
		}

		parallelProcessor.joinReplace(samples, numChannels, numSamples);
#endif
		
	}

	void PluginProcessor::processBlockBypassed(double**, dsp::MidiBuffer&, int, int) noexcept
	{}

	void PluginProcessor::savePatch(arch::State& state)
	{
		keySelector.savePatch(state);
		for(auto i = 0; i < 2; ++i)
		{
			const auto& material = modalFilter.getMaterial(i);
			const auto matStr = "mat" + juce::String(i);
			material.savePatch(state, matStr);
		}
	}

	void PluginProcessor::loadPatch(const arch::State& state)
	{
		keySelector.loadPatch(state);
		for (auto i = 0; i < 2; ++i)
		{
			auto& material = modalFilter.getMaterial(i);
			const auto matStr = "mat" + juce::String(i);
			material.loadPatch(state, matStr);
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