#include "Processor.h"
#include "Editor.h"

#if PPDHasStereoConfig
#include "audio/dsp/MidSide.h"
#endif

#include "arch/Math.h"
#include "audio/dsp/Distortion.h"

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
        
        pluginProcessor(params
#if PPDHasTuningEditor
		, xenManager
#endif
        ),
        audioBufferD(),
        midiSubBuffer(),
        midiOutBuffer(),

        mixProcessor(),
#if PPDHasHQ
        oversampler(),
#endif
        sampleRateUp(0.),
        blockSizeUp(dsp::BlockSize)
    {
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
        pluginProcessor.prepare(sampleRateUp);
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
        pluginProcessor.savePatch();
        params.savePatch(state);
        state.savePatch(*this, destData);
    }

    void Processor::setStateInformation(const void* data, int sizeInBytes)
    {
        state.loadPatch(*this, data, sizeInBytes);
        params.loadPatch(state);
        pluginProcessor.loadPatch();
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
        const auto delta = params(PID::Delta).getValMod() > .5f;
#endif
        const auto gainOutDb = static_cast<double>(params(PID::GainOut).getValModDenorm());

        midiOutBuffer.clear();

#if PPDHasTuningEditor
        auto xen = params(PID::Xen).getValModDenorm();
        auto masterTune = params(PID::MasterTune).getValModDenorm();
        auto referencePitch = params(PID::ReferencePitch).getValModDenorm();
        auto pitchbendRange = params(PID::PitchbendRange).getValModDenorm();
        xenManager(xen, masterTune, referencePitch, pitchbendRange);
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

            processBlockOversampler(samples, midiSubBuffer, numChannels, numSamples);

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

#if JUCE_DEBUG
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
        int numChannels, int numSamples) noexcept
    {
#if PPDHasHQ
        auto bufferInfo = oversampler.upsample(samples, numChannels, numSamples);
        const auto numSamplesUp = bufferInfo.numSamples;
        double* samplesUp[] = { bufferInfo.smplsL, bufferInfo.smplsR };
#else
        double* samplesUp[] = { samples[0], samples[1] };
        const auto numSamplesUp = numSamples;
#endif
        pluginProcessor(samplesUp, midi, numChannels, numSamplesUp);

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