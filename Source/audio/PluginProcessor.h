#pragma once
#include <juce_events/juce_events.h>

#include "../HammerUndMeiszelTests.h"
#include "../param/Param.h"
#include "dsp/KeySelector.h"
#include "dsp/midi/MonophonyHandler.h"
#include "dsp/midi/AutoMPE.h"
#include "dsp/midi/MPESplit.h"
#include "dsp/ParallelProcessor.h"
#include "dsp/NoiseSynth.h"
#include "dsp/hnm/modal/ModalFilter.h"
#include "dsp/hnm/formant/FormantFilter.h"
#include "dsp/hnm/HnmLowpass.h"
#include "dsp/hnm/comb/Comb.h"

//This is where this plugin's custom dsp is implemented
namespace audio
{
	using Timer = juce::Timer;

	struct PluginProcessor :
		public Timer
	{
		using Params = param::Params;
		using PID = param::PID;
		
		PluginProcessor(Params&, arch::XenManager&);

		// sampleRate
		void prepare(double);

		// samples, midiBuffer, bpm, numChannels, numSamples, playing
		void operator()(double**, dsp::MidiBuffer&, double, int, int, bool) noexcept;
		
		// samples, midiBuffer, numChannels, numSamples
		void processBlockBypassed(double**, dsp::MidiBuffer&, int, int) noexcept;

		void savePatch(arch::State&);
		
		void loadPatch(const arch::State&);

		void timerCallback() override;

		Params& params;
		arch::XenManager& xen;
		double sampleRate;

		dsp::KeySelector keySelector;
		dsp::MonophonyHandler monophonyHandler;
		dsp::AutoMPE autoMPE;
		dsp::MPESplit voiceSplit;
		dsp::PPMIDIBand parallelProcessor;
		std::array<std::array<double, dsp::BlockSize>, 2> formantLayer;

		dsp::EnvGenMultiVoice envGensAmp, envGensMod;

		dsp::NoiseSynth noiseSynth;
		dsp::modal::ModalFilter modalFilter;
		dsp::formant::Filter formantFilter;
		dsp::hnm::Comb combFilter;
		dsp::hnm::lp::Filter lowpass;

		std::atomic<bool> editorExists;

		std::atomic<int> recording;
		int recSampleIndex;
	};
}