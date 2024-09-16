#include "PluginProcessor.h"
#include <juce_core/juce_core.h>
//#include "../HammerUndMeiszelTests.h"

namespace audio
{
	PluginProcessor::PluginProcessor(Params& _params, const arch::XenManager& _xen) :
		params(_params), xen(_xen), sampleRate(1.),
		autoMPE(), voiceSplit(), parallelProcessor(),
		envGensAmp(), envGensMod(),
		modalFilter(), flanger(xen),
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
		envGensAmp.prepare(sampleRate);
		envGensMod.prepare(sampleRate);
	}

	void PluginProcessor::operator()(double** samples, dsp::MidiBuffer& midi, int numChannels, int numSamples) noexcept
	{
		//randNoiseGen(samples, numChannels, numSamples, .25);
		//randMeloGen(midi, numSamples);
		autoMPE(midi); voiceSplit(midi);

		const auto& modalOctParam = params(PID::ModalOct);
		const auto& modalSemiParam = params(PID::ModalSemi);
		auto modalSemi = static_cast<double>(std::round(modalSemiParam.getValModDenorm()));
		modalSemi += static_cast<double>(std::round(modalOctParam.getValModDenorm())) * xen.getXen();

		const auto& modalSmartKeytrackParam = params(PID::ModalSmartKeytrack);
		const auto modalSmartKeytrack = static_cast<double>(modalSmartKeytrackParam.getValMod());
		const auto& modalSmartKeytrackEnvParam = params(PID::ModalSmartKeytrackEnv);
		const auto modalSmartKeytrackEnv = static_cast<double>(modalSmartKeytrackEnvParam.getValModDenorm());
		const auto& modalSmartKeytrackBreiteParam = params(PID::ModalSmartKeytrackBreite);
		const auto modalSmartKeytrackBreite = static_cast<double>(modalSmartKeytrackBreiteParam.getValModDenorm());

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

		/*
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
		*/

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

		const dsp::modal2::Voice::Parameters modalVoiceParams
		(
			modalSmartKeytrack, modalBlend, modalSpreizung, modalHarmonie, modalKraft, modalReso, modalResoDamp,
			modalSmartKeytrackEnv, modalBlendEnv, modalSpreizungEnv, modalHarmonieEnv, modalKraftEnv, modalResoEnv, modalResoDampEnv,
			modalSmartKeytrackBreite, modalBlendBreite, modalSpreizungBreite, modalHarmonieBreite, modalKraftBreite, modalResoBreite, modalResoDampBreite
		);

		envGensAmp.updateParameters({ envGenAmpAttack, envGenAmpDecay, envGenAmpSustain, envGenAmpRelease });
		envGensMod.updateParameters({ envGenModAttack, envGenModDecay, envGenModSustain, envGenModRelease });

		modalFilter();

		//flanger.synthesizeWHead(numSamples);
		//flanger.updateParameters
		//(
		//	combFeedback, combAPShape, numChannels
		//);
		
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
			
			// process modal filter
			modalFilter
			(
				samplesVoice,
				midiVoice, xen,
				modalVoiceParams,
				modalSemi, envGenModVal,
				numChannels, numSamples,
				v
			);

			/*
			flanger
			(
				samplesVoice, midiVoice, combSemi,
				combAPResonanz,
				numChannels, numSamples,
				v
			);
			*/

			const bool voiceSilent = math::bufferSilent(samplesVoice, numChannels, numSamples);
			const bool sleepy = !envGenAmpInfo.active && voiceSilent;
			parallelProcessor.setSleepy(sleepy, v);
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
			material.savePatch(state, "mat" + juce::String(i));
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
				const auto freqVal = state.get(peakStr + "fr");
				if (freqVal != nullptr)
					peakInfo.freqHz = static_cast<double>(*freqVal);
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