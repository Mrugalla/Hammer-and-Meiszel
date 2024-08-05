#pragma once
#include <juce_events/juce_events.h>

#include "../HammerUndMeiszelTests.h"
#include "../param/Param.h"
#include "dsp/midi/AutoMPE.h"
#include "dsp/midi/MPESplit.h"
#include "dsp/ParallelProcessor.h"
#include "dsp/modal2/ModalFilter.h"
#include "dsp/flanger/flanger.h"

//This is where this plugin's custom dsp is implemented
namespace audio
{
	using Timer = juce::Timer;

	struct PluginProcessor :
		public Timer
	{
		using Params = param::Params;
		using PID = param::PID;
		
		PluginProcessor(Params&, const arch::XenManager&);

		/* sampleRate */
		void prepare(double);

		/* samples, midiBuffer, numChannels, numSamples */
		void operator()(double**, dsp::MidiBuffer&, int, int) noexcept;
		
		/* samples, midiBuffer, numChannels, numSamples */
		void processBlockBypassed(double**, dsp::MidiBuffer&, int, int) noexcept;

		void savePatch(arch::State&);
		
		void loadPatch(const arch::State&);

		void timerCallback() override;

		Params& params;
		const arch::XenManager& xen;
		double sampleRate;

		//test::RandomNoiseGenerator randNoiseGen;
		//test::MIDIRandomMelodyGenerator randMeloGen;

		dsp::AutoMPE autoMPE;
		dsp::MPESplit voiceSplit;
		dsp::PPMIDIBand parallelProcessor;

		dsp::EnvGenMultiVoice envGensAmp, envGensMod;

		dsp::modal2::ModalFilter modalFilter;
		dsp::flanger::Flanger flanger;
		std::atomic<bool> editorExists;
	};
}