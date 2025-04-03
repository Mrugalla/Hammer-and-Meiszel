#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "audio/PluginProcessor.h"

#include "arch/XenManager.h"
#include "param/Param.h"
#include "audio/dsp/MixProcessor.h"
#include "audio/dsp/Oversampler.h"
#include "audio/dsp/PluginRecorder.h"

namespace audio
{
    using ChannelSet = juce::AudioChannelSet;
    using MidiBuffer = juce::MidiBuffer;
	using AudioBufferF = juce::AudioBuffer<float>;
    using AudioBufferD = juce::AudioBuffer<double>;
    using Timer = juce::Timer;
    using SIMD = juce::FloatVectorOperations;
    
	using XenManager = arch::XenManager;
    using State = arch::State;
    using Param = param::Param;
    using Params = param::Params;
    using PID = param::PID;
    using PRM = dsp::PRMD;
    
    struct Processor :
        public juce::AudioProcessor,
        public Timer
    {
        using BusesProps = juce::AudioProcessor::BusesProperties;
        using ValueTree = juce::ValueTree;

        BusesProps makeBusesProps();
        bool canAddBus(bool) const override;
		
        Processor();
        ~Processor() override;
        void prepareToPlay(double, int) override;
        void releaseResources() override;
        bool isBusesLayoutSupported(const BusesLayout&) const override;
        
        void processBlock(AudioBufferF&, MidiBuffer&) override;
        void processBlockBypassed(AudioBufferF&, MidiBuffer&) override;
        void processBlock(AudioBufferD&, MidiBuffer&) override;
        void processBlockBypassed(AudioBufferD&, MidiBuffer&) override;

        void processBlockOversampler(double* const*, MidiBuffer&, const dsp::Transport::Info&, int, int) noexcept;
        
        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override;
        const juce::String getName() const override;
        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect() const override;
        double getTailLengthSeconds() const override;
        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram(int) override;
        const juce::String getProgramName(int) override;
        void changeProgramName(int, const juce::String&) override;
        void getStateInformation(juce::MemoryBlock&) override;
        void setStateInformation(const void*, int) override;
        void timerCallback() override;
        bool supportsDoublePrecisionProcessing() const override;
        void forcePrepare();

#if PPDHasTuningEditor
        XenManager xenManager;
#endif
        Params params;
        State state;

        dsp::Transport transport;
        PluginProcessor pluginProcessor;
        AudioBufferD audioBufferD;
        MidiBuffer midiSubBuffer, midiOutBuffer;

        dsp::MixProcessor mixProcessor;
        std::array<juce::dsp::FirstOrderTPTFilter<float>, 2> highpasses;
        dsp::PluginRecorder recorder;
#if PPDHasHQ
        dsp::Oversampler oversampler;
#endif
        double sampleRateUp;
        int blockSizeUp;

        //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
}
