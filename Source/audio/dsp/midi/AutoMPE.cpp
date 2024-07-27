#include "AutoMPE.h"

namespace dsp
{
	AutoMPE::Voice::Voice(int _note, int _channel, bool _noteOn) :
		note(_note),
		channel(_channel),
		noteOn(_noteOn)
	{}

	AutoMPE::AutoMPE() :
		buffer(),
		voices(),
		channelIdx(-1)
	{}

	const AutoMPE::Voices& AutoMPE::AutoMPE::getVoices() const noexcept
	{
		return voices;
	}

	void AutoMPE::operator()(MidiBuffer& midi)
	{
		buffer.clear();

		for (const auto it : midi)
		{
			auto msg = it.getMessage();

			if (msg.isNoteOn())
				processNoteOn(msg, it.samplePosition);
			else if (msg.isNoteOff())
				processNoteOff(msg);
			else if (msg.isPitchWheel())
				processPitchWheel(msg, it.samplePosition);
			else
				msg.setChannel(1);

			buffer.addEvent(msg, it.samplePosition);
		}

		midi.swapWith(buffer);
	}

	void AutoMPE::incChannelIdx() noexcept
	{
		channelIdx = (channelIdx + 1) % voices.size();
	}

	void AutoMPE::processNoteOn(MidiMessage& msg, int ts)
	{
		for (auto ch = 0; ch < VoicesSize; ++ch)
		{
			incChannelIdx();
			auto& voice = voices[channelIdx];
			if (!voice.noteOn)
				return processNoteOn(voice, msg);
		}
		incChannelIdx();
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
		for (auto ch = 0; ch < VoicesSize; ++ch)
		{
			auto i = channelIdx - ch;
			if (i < 0)
				i += VoicesSize;

			auto& voice = voices[i];

			if (voice.noteOn && voice.note == msg.getNoteNumber())
				return processNoteOff(voice, msg);
		}
	}

	void AutoMPE::processNoteOff(Voice& voice, MidiMessage& msg) noexcept
	{
		msg.setChannel(voice.channel);
		voice.note = 0;
		voice.noteOn = false;
	}

	void AutoMPE::processPitchWheel(MidiMessage& msg, int ts) noexcept
	{
		for (auto ch = 0; ch < VoicesSize; ++ch)
		{
			auto& voice = voices[ch];
			msg.setChannel(voice.channel);
			buffer.addEvent(msg, ts);
		}
	}
}