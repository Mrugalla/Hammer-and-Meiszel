#pragma once
#include "../param/Param.h"

#include "../HammerUndMeiszelTests.h"

#include "dsp/midi/AutoMPE.h"
#include "dsp/midi/MPESplit.h"
#include "dsp/ParallelProcessor.h"
#include "dsp/ModalFilter.h"

/*
This is where dsp custom to each individual plugin will be written
*/

namespace audio
{
	struct PluginProcessor
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

		Params& params;
		const arch::XenManager& xen;
		double sampleRate;

		test::MIDIRandomMelodyGenerator randomMelodyGenerator;

		dsp::AutoMPE autoMPE;
		dsp::MPESplit voiceSplit;
		dsp::PPMIDIBand parallelProcessor;
		dsp::modal::Filter modalFilter;
	};
}