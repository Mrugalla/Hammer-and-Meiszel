#pragma once
#include <juce_events/juce_events.h>

#include "../param/Param.h"

#include "../HammerUndMeiszelTests.h"

#include "dsp/midi/AutoMPE.h"
#include "dsp/midi/MPESplit.h"
#include "dsp/ParallelProcessor.h"
#include "dsp/modal/Filtr.h"
#include "dsp/DelayVoices.h"

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

		void savePatch();
		
		void loadPatch();

		void timerCallback() override;

		Params& params;
		const arch::XenManager& xen;
		double sampleRate;

		dsp::AutoMPE autoMPE;
		dsp::MPESplit voiceSplit;
		dsp::PPMIDIBand parallelProcessor;
		dsp::modal::Filtr modalFilter;
		dsp::CombFilterVoices combFilter;
		std::atomic<bool> editorExists;
	};
}