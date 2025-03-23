#pragma once
#include "../../Using.h"

namespace dsp
{
	struct AutoMPE
	{
		using uint8 = juce::uint8;

		// note (-1 means noteOff), channel
		struct Voice
		{
			Voice(int = -1, int = 1);

			int note, channel;
		};

		static constexpr int VoicesSize = NumMPEChannels;
		using Voices = std::array<Voice, VoicesSize>;

		AutoMPE();

		// midi, poly
		void operator()(MidiBuffer&, int);

		const Voices& getVoices() const noexcept;

	private:
		MidiBuffer buffer;
		Voices voices;
		int channelIdx, poly;

		void updatePoly(int);

		void processBlock(const MidiBuffer&);

		void incChannelIdx() noexcept;

		// msg, ts
		void processNoteOn(MidiMessage&, int);

		// voice, msg, ts
		void processNoteOn(Voice&, MidiMessage&, int) noexcept;

		// msg, ts
		void processNoteOff(MidiMessage&, int) noexcept;

		// voice, msg, ts
		void processNoteOff(Voice&, MidiMessage&, int) noexcept;

		// msg, ts
		void processPitchWheel(MidiMessage&, int) noexcept;
	};
}