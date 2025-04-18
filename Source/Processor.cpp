#include "Processor.h"
#include "Editor.h"

#if PPDHasStereoConfig
#include "audio/dsp/MidSide.h"
#endif

#include "arch/Math.h"
#include "audio/dsp/Distortion.h"

#define KeepState true

namespace audio
{
    Processor::BusesProps Processor::makeBusesProps()
    {
        BusesProps bp;
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        bp.addBus(true, "Input", ChannelSet::stereo(), true);
#endif
        bp.addBus(false, "Output", ChannelSet::stereo(), true);
#if PPDHasSidechain
        if (!juce::JUCEApplicationBase::isStandaloneApp())
            bp.addBus(true, "Sidechain", ChannelSet::stereo(), true);
#endif
#endif
        return bp;
    }
	
    Processor::Processor() :
        juce::AudioProcessor(makeBusesProps()),
		Timer(),
#if PPDHasTuningEditor
		xenManager(),
        params(*this, xenManager),
#else
		params(*this),
#endif
        state(),
        
        transport(),
        pluginProcessor(params
#if PPDHasTuningEditor
		, xenManager
#endif
        ),
        audioBufferD(),
        midiSubBuffer(),
        midiOutBuffer(),

        mixProcessor(),
        highpasses(),
        recorder(),
#if PPDHasHQ
        oversampler(),
#endif
        sampleRateUp(0.),
        blockSizeUp(dsp::BlockSize)
    {
        const auto& user = *state.props.getUserSettings();
        const auto& settingsFile = user.getFile();
        const auto settingsDirectory = settingsFile.getParentDirectory();
		const auto patchesDirectory = settingsDirectory.getChildFile("Patches");
        if (!patchesDirectory.exists())
        {
            patchesDirectory.createDirectory();

			using String = juce::String;
            const auto makePatch = [&p = patchesDirectory](const String& name, const void* data, int size)
            {
                const auto initFile = p.getChildFile(name + ".txt");
                if (initFile.existsAsFile())
                    initFile.deleteFile();
                initFile.create();
                initFile.replaceWithData(data, size);
            };
            makePatch("Dematerialisiere", BinaryData::Dematerialisiere_txt, BinaryData::Dematerialisiere_txtSize);
			makePatch("Doktor Tropfsteinhoele", BinaryData::Doktor_Tropfsteinhoele_txt, BinaryData::Doktor_Tropfsteinhoele_txtSize);
			makePatch("Zerbrochen", BinaryData::Zerbrochen_txt, BinaryData::Zerbrochen_txtSize);
            makePatch("Tingeling", BinaryData::Tingeling_txt, BinaryData::Tingeling_txtSize);
			makePatch("Slickmap", BinaryData::Slickmap_txt, BinaryData::Slickmap_txtSize);
			makePatch("Black Hole", BinaryData::Black_Hole_txt, BinaryData::Black_Hole_txtSize);
			makePatch("Dark Morning", BinaryData::Dark_Morning_txt, BinaryData::Dark_Morning_txtSize);
			makePatch("Ensamble", BinaryData::Ensamble_txt, BinaryData::Ensamble_txtSize);
			makePatch("Fake Reverb", BinaryData::Fake_Reverb_txt, BinaryData::Fake_Reverb_txtSize);
			makePatch("Hat Sweetener", BinaryData::Hat_Sweetener_txt, BinaryData::Hat_Sweetener_txtSize);
			makePatch("Percussive Particles", BinaryData::Percussive_Particles_txt, BinaryData::Percussive_Particles_txtSize);
			makePatch("Robo Madness", BinaryData::Robo_Madness_txt, BinaryData::Robo_Madness_txtSize);
			makePatch("Shooting Star", BinaryData::Shooting_Star_txt, BinaryData::Shooting_Star_txtSize);
			makePatch("Sweet Night", BinaryData::Sweet_Night_txt, BinaryData::Sweet_Night_txtSize);
			makePatch("Unstable Identity", BinaryData::Unstable_Identity_txt, BinaryData::Unstable_Identity_txtSize);
			makePatch("Orgelklang", BinaryData::Orgelklang_txt, BinaryData::Orgelklang_txtSize);
		}

        state.set("author", "factory");
        params.savePatch(state);
        pluginProcessor.savePatch(state);
        const auto init = state.state.createCopy();
        const auto initFile = patchesDirectory.getChildFile(" init .txt");
        if (initFile.existsAsFile())
            initFile.deleteFile();
        initFile.create();
		initFile.replaceWithText(init.toXmlString());

        for (auto& highpass : highpasses)
            highpass.setType(juce::dsp::FirstOrderTPTFilterType::highpass);
    }

    Processor::~Processor()
    {
        auto& user = *state.props.getUserSettings();
        user.setValue("firstTimeUwU", false);
        user.save();
    }

    bool Processor::supportsDoublePrecisionProcessing() const
    {
        return true;
    }
	
    bool Processor::canAddBus(bool isInput) const
    {
        if (wrapperType == wrapperType_Standalone)
            return false;

        return PPDHasSidechain ? isInput : false;
    }
	
    const juce::String Processor::getName() const
    {
        return JucePlugin_Name;
    }

    bool Processor::acceptsMidi() const
    {
#if JucePlugin_WantsMidiInput
        return true;
#else
        return false;
#endif
    }

    bool Processor::producesMidi() const
    {
#if JucePlugin_ProducesMidiOutput
        return true;
#else
        return false;
#endif
    }

    bool Processor::isMidiEffect() const
    {
#if JucePlugin_IsMidiEffect
        return true;
#else
        return false;
#endif
    }

    double Processor::getTailLengthSeconds() const
    {
        return 0.;
    }

    int Processor::getNumPrograms()
    {
        return 1;
    }

    int Processor::getCurrentProgram()
    {
        return 0;
    }

    void Processor::setCurrentProgram(int)
    {
    }

    const juce::String Processor::getProgramName(int)
    {
        return {};
    }

    void Processor::changeProgramName(int, const juce::String&)
    {
    }

    void Processor::prepareToPlay(double sampleRate, int maxBlockSize)
    {
        auto latency = 0;

        audioBufferD.setSize(2, maxBlockSize, false, true, false);
        mixProcessor.prepare(sampleRate);
#if PPDHasHQ
        const auto hqEnabled = params(PID::HQ).getValMod() > .5f;
        oversampler.prepare(sampleRate, hqEnabled);
        latency += oversampler.getLatency();
        sampleRateUp = oversampler.sampleRateUp;
        blockSizeUp = oversampler.enabled ? dsp::BlockSize2x : dsp::BlockSize;
#else
        sampleRateUp = sampleRate;
		blockSizeUp = dsp::BlockSize;
#endif
        transport.prepare(1. / sampleRate);
        pluginProcessor.prepare(sampleRateUp);
        juce::dsp::ProcessSpec spec;
		spec.sampleRate = sampleRateUp;
		spec.maximumBlockSize = blockSizeUp;
		spec.numChannels = 2;
        for (auto& highpass : highpasses)
        {
            highpass.reset();
            highpass.prepare(spec);
            highpass.setCutoffFrequency(20.f);
        }
        recorder.prepare(sampleRate);
        setLatencySamples(latency);
        startTimerHz(4);
    }

    void Processor::releaseResources()
    {
    }

    bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
#if JucePlugin_IsMidiEffect
        juce::ignoreUnused(layouts);
        return true;
#endif
        const auto mono = ChannelSet::mono();
        const auto stereo = ChannelSet::stereo();

        const auto mainIn = layouts.getMainInputChannelSet();
        const auto mainOut = layouts.getMainOutputChannelSet();

        if (mainIn != mainOut)
            return false;

        if (mainOut != stereo && mainOut != mono)
            return false;

#if PPDHasSidechain
        if (wrapperType != wrapperType_Standalone)
        {
            const auto scIn = layouts.getChannelSet(true, 1);
            if (!scIn.isDisabled())
                if (scIn != mono && scIn != stereo)
                    return false;
        }
#endif

        return true;
    }

    bool Processor::hasEditor() const
    {
        return true;
    }

    juce::AudioProcessorEditor* Processor::createEditor()
    {
        return new gui::Editor(*this);
    }

    void Processor::getStateInformation(juce::MemoryBlock& destData)
    {
#if KeepState
        pluginProcessor.savePatch(state);
        params.savePatch(state);
        state.savePatch(*this, destData);
#endif
    }

    void Processor::setStateInformation(const void* data, int sizeInBytes)
    {
#if KeepState
        state.loadPatch(*this, data, sizeInBytes);
        params.loadPatch(state);
        pluginProcessor.loadPatch(state);
#endif
    }

    void Processor::processBlockBypassed(AudioBufferD& buffer, MidiBuffer& midiMessages)
    {
        juce::ScopedNoDenormals noDenormals;
		
        const auto macroVal = params(PID::Macro).getValue();
        params.modulate(macroVal);
		
        const auto numSamplesMain = buffer.getNumSamples();
        if (numSamplesMain == 0)
            return;
		
        const auto numChannels = buffer.getNumChannels() == 2 ? 2 : 1;
        auto samplesMain = buffer.getArrayOfWritePointers();

        for (auto s = 0; s < numSamplesMain; s += dsp::BlockSize)
        {
            double* samples[] = { &samplesMain[0][s], &samplesMain[1][s] };
            const auto dif = numSamplesMain - s;
            const auto numSamples = dif < dsp::BlockSize ? dif : dsp::BlockSize;

            pluginProcessor.processBlockBypassed(samples, midiMessages, numChannels, numSamples);
        }
    }

    void Processor::processBlockBypassed(AudioBufferF& buffer, MidiBuffer& midiMessages)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        audioBufferD.setSize(numChannels, numSamples, true, false, true);

        auto samplesF = buffer.getArrayOfWritePointers();
        auto samplesD = audioBufferD.getArrayOfWritePointers();

        for (auto ch = 0; ch < numChannels; ++ch)
        {
            const auto smplsF = samplesF[ch];
            auto smplsD = samplesD[ch];

            for (auto s = 0; s < numSamples; ++s)
                smplsD[s] = static_cast<double>(smplsF[s]);
        }

        processBlockBypassed(audioBufferD, midiMessages);

        for (auto ch = 0; ch < numChannels; ++ch)
        {
            auto smplsF = samplesF[ch];
            const auto smplsD = samplesD[ch];

            for (auto s = 0; s < numSamples; ++s)
                smplsF[s] = static_cast<float>(smplsD[s]);
        }
    }

    void Processor::processBlock(AudioBufferD& buffer, MidiBuffer& midiMessages)
    {
        const bool pluginDisabled = params(PID::Power).getValue() < .5f;
        if (pluginDisabled)
            return processBlockBypassed(buffer, midiMessages);
		
        juce::ScopedNoDenormals noDenormals;
		
        const auto macroVal = params(PID::Macro).getValue();
        params.modulate(macroVal);

        const auto numSamplesMain = buffer.getNumSamples();
        {
            const auto totalNumInputChannels = getTotalNumInputChannels();
            const auto totalNumOutputChannels = getTotalNumOutputChannels();
            for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
                buffer.clear(i, 0, numSamplesMain);
        }
        if (numSamplesMain == 0)
            return;
		
        const auto numChannels = buffer.getNumChannels();
		auto samplesMain = buffer.getArrayOfWritePointers();

        transport(playHead);
#if PPDHasStereoConfig
        bool midSide = false;
        if (numChannels == 2)
        {
            midSide = params(PID::StereoConfig).getValue() > .5f;
            if(midSide)
                dsp::midSideEncode(samplesMain, numSamplesMain);
        }
#endif

#if PPDIsNonlinear
        const auto gainInDb = static_cast<double>(params(PID::GainIn).getValModDenorm());
        const auto unityGain = params(PID::UnityGain).getValMod() > .5f;
#endif
#if PPDIO == PPDIODryWet
        const auto gainDryDb = static_cast<double>(params(PID::GainDry).getValModDenorm());
        const auto gainWetDb = static_cast<double>(params(PID::GainWet).getValModDenorm());
#elif PPDIO == PPDIOWetMix
        const auto gainWetDb = static_cast<double>(params(PID::GainWet).getValModDenorm());
        const auto mix = static_cast<double>(params(PID::Mix).getValMod());
#if PPDHasDelta
        const auto delta = params(PID::Delta).getValMod() > .5f;
#else
        const bool delta = false;
#endif
#endif
        const auto gainOutDb = static_cast<double>(params(PID::GainOut).getValModDenorm());

        midiOutBuffer.clear();

#if PPDHasTuningEditor
        auto xen = params(PID::Xen).getValModDenorm();
		const auto xenSnap = params(PID::XenSnap).getValMod() > .5f;
        if(xenSnap)
			xen = std::round(xen);
        const auto masterTune = std::round(params(PID::MasterTune).getValModDenorm());
        const auto anchor = std::round(params(PID::AnchorPitch).getValModDenorm());
        const auto pitchbendRange = std::round(params(PID::PitchbendRange).getValModDenorm());
        xenManager({ xen, masterTune, anchor, pitchbendRange }, numChannels);
#endif

        for (auto s = 0; s < numSamplesMain; s += dsp::BlockSize)
        {
            double* samples[] = { &samplesMain[0][s], &samplesMain[1][s] };
            const auto dif = numSamplesMain - s;
            const auto numSamples = dif < dsp::BlockSize ? dif : dsp::BlockSize;

#if PPDIO == PPDIOOut
	#if PPDIsNonlinear
			mixProcessor.split
			(
				samples, gainInDb, numChannels, numSamples
			);
	#endif
#elif PPDIO == PPDIODryWet
    #if PPDIsNonlinear
            mixProcessor.split
            (
                samples, gainDryDb, gainInDb, numChannels, numSamples
            );
    #else
            mixProcessor.split
            (
                samples, gainDryDb, numChannels, numSamples
            );
    #endif
#else
    #if PPDIsNonlinear
            mixProcessor.split
            (
                samples, gainInDb, numChannels, numSamples
            );
    #else
            mixProcessor.split
            (
                samples, numChannels, numSamples
            );
    #endif
#endif
            midiSubBuffer.clear();
            for(auto it : midiMessages)
			{
                const auto ts = it.samplePosition;
				if (ts >= s && ts < s + numSamples)
                    midiSubBuffer.addEvent(it.getMessage(), ts - s);
			}

            processBlockOversampler(samples, midiSubBuffer, transport.info, numChannels, numSamples);
            transport(numSamples);

            for(auto it : midiSubBuffer)
                midiOutBuffer.addEvent(it.getMessage(), it.samplePosition + s);
            
#if PPDIO == PPDIOOut
    #if PPDIsNonlinear
			mixProcessor.join
			(
				samples, gainOutDb, numChannels, numSamples, unityGain
			);
	#else
            mixProcessor.join
			(
				samples, gainOutDb, numChannels, numSamples
			);
    #endif
#elif PPDIO == PPDIODryWet
    #if PPDIsNonlinear
            mixProcessor.join
            (
                samples, gainWetDb, gainOutDb, numChannels, numSamples, unityGain
            );
    #else
            mixProcessor.join
            (
                samples, gainWetDb, gainOutDb, numChannels, numSamples
            );
    #endif
#else
    #if PPDIsNonlinear
            mixProcessor.join
            (
                samples, mix, gainWetDb, gainOutDb, numChannels, numSamples, unityGain, delta
            );
    #else
            mixProcessor.join
            (
                samples, mix, gainWetDb, gainOutDb, numChannels, numSamples, delta
            );
    #endif
#endif
        }

        midiMessages.swapWith(midiOutBuffer);

#if PPDHasStereoConfig
		if (midSide)
			dsp::midSideDecode(samplesMain, numSamplesMain);
#endif
#if PPDDCOffsetFilter != 0
        for (auto& highpass : highpasses)
        {
            juce::dsp::AudioBlock<double> block(buffer);
            juce::dsp::ProcessContextReplacing<double> context(block);
            highpass.process(context);
        }
#endif

		const auto& softClipParam = params(PID::SoftClip);
		const auto softClip = softClipParam.getValMod() > .5f;
        const auto knee = .5 / dsp::Pi;
        if(softClip)
        {
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samplesMain[ch];
                for (auto s = 0; s < numSamplesMain; ++s)
                    smpls[s] = dsp::softclipPrismaHeavy(smpls[s], 1., knee);
			}
		}

        recorder(samplesMain, numChannels, numSamplesMain);

#if JUCE_DEBUG && false
        for (auto ch = 0; ch < numChannels; ++ch)
        {
            auto smpls = samplesMain[ch];
            for (auto s = 0; s < numSamplesMain; ++s)
                smpls[s] = dsp::hardclip(smpls[s], 2.);
        }
#endif
    }

    void Processor::processBlock(AudioBufferF& buffer, MidiBuffer& midiMessages)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        audioBufferD.setSize(numChannels, numSamples, true, false, true);

        auto samplesF = buffer.getArrayOfWritePointers();
        auto samplesD = audioBufferD.getArrayOfWritePointers();

        for (auto ch = 0; ch < numChannels; ++ch)
        {
            const auto smplsF = samplesF[ch];
            auto smplsD = samplesD[ch];

            for (auto s = 0; s < numSamples; ++s)
                smplsD[s] = static_cast<double>(smplsF[s]);
        }

        processBlock(audioBufferD, midiMessages);

        for (auto ch = 0; ch < numChannels; ++ch)
        {
            auto smplsF = samplesF[ch];
            const auto smplsD = samplesD[ch];

            for (auto s = 0; s < numSamples; ++s)
                smplsF[s] = static_cast<float>(smplsD[s]);
        }
    }

    void Processor::processBlockOversampler(double* const* samples, MidiBuffer& midi,
        const dsp::Transport::Info& transportInfo, int numChannels, int numSamples) noexcept
    {
#if PPDHasHQ
        auto bufferInfo = oversampler.upsample(samples, numChannels, numSamples);
        const auto numSamplesUp = bufferInfo.numSamples;
        double* samplesUp[] = { bufferInfo.smplsL, bufferInfo.smplsR };
#else
        double* samplesUp[] = { samples[0], samples[1] };
        const auto numSamplesUp = numSamples;
#endif
        pluginProcessor(samplesUp, midi, transportInfo, numChannels, numSamplesUp);
#if PPDHasHQ
        oversampler.downsample(samples, numSamples);
#endif
    }

    void Processor::timerCallback()
    {
        bool needForcePrepare = false;
#if PPDHasHQ
		const auto hqEnabled = params(PID::HQ).getValMod() > .5f;
        if (oversampler.enabled != hqEnabled)
            needForcePrepare = true;
#endif
        if(needForcePrepare)
            forcePrepare();
    }

    void Processor::forcePrepare()
    {
        suspendProcessing(true);
        prepareToPlay(getSampleRate(), getBlockSize());
        suspendProcessing(false);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new audio::Processor();
}

#undef KeepState