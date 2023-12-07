#include "AutoMPE.h"

namespace dsp
{
	AutoMPE::Voice::Voice() :
		note(-1),
		channel(0),
		noteOn(false)
	{}

	AutoMPE::AutoMPE() :
		buffer(),
		voices(),
		channelIdx(0)
	{}

	void AutoMPE::operator()(MidiBuffer& midiMessages)
	{
		buffer.clear();

		for (const auto it : midiMessages)
		{
			auto msg = it.getMessage();

			if (msg.isNoteOn())
				processNoteOn(msg, it.samplePosition);
			else if (msg.isNoteOff())
				processNoteOff(msg);

			buffer.addEvent(msg, it.samplePosition);
		}

		midiMessages.swapWith(buffer);
	}

	void AutoMPE::incChannel() noexcept
	{
		channelIdx = (channelIdx + 1) & NumMPEChannels;
	}

	void AutoMPE::processNoteOn(MidiMessage& msg, int ts)
	{
		for (auto ch = 0; ch < NumMPEChannels; ++ch)
		{
			incChannel();
			auto& voice = voices[channelIdx];
			if (!voice.noteOn)
				return processNoteOn(voice, msg);
		}
		incChannel();
		auto& voice = voices[channelIdx];
		buffer.addEvent(MidiMessage::noteOff(voice.channel, voice.note), ts);
		processNoteOn(voice, msg);
	}

	void AutoMPE::processNoteOn(Voice& voice, MidiMessage& msg) noexcept
	{
		const auto note = msg.getNoteNumber();
		voice.note = note;
		voice.channel = channelIdx + 2;
		voice.noteOn = true;
		msg.setChannel(voice.channel);
	}

	void AutoMPE::processNoteOff(MidiMessage& msg) noexcept
	{
		for (auto ch = 0; ch < NumMPEChannels; ++ch)
		{
			auto i = channelIdx - ch;
			if (i < 0)
				i += NumChannels;

			auto& voice = voices[i];

			if (voice.noteOn && voice.note == msg.getNoteNumber())
				return processNoteOff(msg);
		}
	}

	void AutoMPE::processNoteOff(Voice& voice, MidiMessage& msg) noexcept
	{
		msg.setChannel(voice.channel);
		voice.note = 0;
		voice.noteOn = false;
	}
}