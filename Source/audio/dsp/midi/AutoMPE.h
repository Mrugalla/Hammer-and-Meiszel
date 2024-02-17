#pragma once
#include "../../Using.h"

namespace dsp
{
	struct AutoMPE
	{
		struct Voice
		{
			Voice(int = -1, int = 1, bool = false);

			int note, channel;
			bool noteOn;
		};

		static constexpr int VoicesSize = NumMPEChannels;
		using Voices = std::array<Voice, VoicesSize>;

		AutoMPE();

		void operator()(MidiBuffer&);

	private:
		MidiBuffer buffer;
		Voices voices;
		int channelIdx;

		void incChannelIdx() noexcept;

		/* msg, ts */
		void processNoteOn(MidiMessage&, int);

		void processNoteOn(Voice&, MidiMessage&) noexcept;

		void processNoteOff(MidiMessage&) noexcept;

		void processNoteOff(Voice&, MidiMessage&) noexcept;

		void processPitchWheel(MidiMessage&, int) noexcept;

	};
}