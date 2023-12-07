#pragma once
#include "../../Using.h"

namespace dsp
{
	struct AutoMPE
	{
		struct Voice
		{
			Voice();

			int note, channel;
			bool noteOn;
		};

		using Voices = std::array<Voice, NumMPEChannels>;

		AutoMPE();

		void operator()(MidiBuffer&);

	private:
		MidiBuffer buffer;
		Voices voices;
		int channelIdx;

		void incChannel() noexcept;

		/* msg, ts */
		void processNoteOn(MidiMessage&, int);

		void processNoteOn(Voice&, MidiMessage&) noexcept;

		void processNoteOff(MidiMessage&) noexcept;

		void processNoteOff(Voice&, MidiMessage&) noexcept;

	};
}