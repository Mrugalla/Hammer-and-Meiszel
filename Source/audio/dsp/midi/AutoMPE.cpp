#include "AutoMPE.h"

namespace dsp
{
	AutoMPE::Voice::Voice(int _note, int _channel) :
		note(_note),
		channel(_channel)
	{}

	AutoMPE::AutoMPE() :
		buffer(),
		voices(),
		channelIdx(-1),
		poly(VoicesSize),
		heldNotes(),
		curNote(-1)
	{}

	const AutoMPE::Voices& AutoMPE::AutoMPE::getVoices() const noexcept
	{
		return voices;
	}

	void AutoMPE::operator()(MidiBuffer& midi, int _poly)
	{
		buffer.clear();
		updatePoly(_poly);
		processBlock(midi);
		midi.swapWith(buffer);
	}

	void AutoMPE::updatePoly(int _poly)
	{
		if (poly == _poly)
			return;
		for (auto v = _poly; v < poly; ++v)
		{
			auto& voice = voices[v];
			if (voice.note != -1)
			{
				buffer.addEvent(MidiMessage::noteOff(voice.channel, voice.note), 0);
				voice.note = -1;
			}
		}
		channelIdx = curNote = -1;
		poly = _poly;
		if(poly == 1)
			for (auto i = 0; i < 128; ++i)
				heldNotes[i] = 0;
	}

	void AutoMPE::processBlock(const MidiBuffer& midi)
	{
		for (const auto it : midi)
		{
			auto msg = it.getMessage();
			const auto ts = it.samplePosition;
			if (msg.isNoteOn())
				processNoteOn(msg, ts);
			else if (msg.isNoteOff())
				processNoteOff(msg, ts);
			else if (msg.isPitchWheel())
				processPitchWheel(msg, ts);
			else
			{
				msg.setChannel(1);
				buffer.addEvent(msg, ts);
			}
		}
	}

	void AutoMPE::incChannelIdx() noexcept
	{
		++channelIdx;
		if(channelIdx >= poly)
			channelIdx = 0;
	}

	void AutoMPE::processNoteOn(MidiMessage& msg, int ts)
	{
		for (auto ch = 0; ch < poly; ++ch)
		{
			incChannelIdx();
			auto& voice = voices[channelIdx];
			const bool voiceAvailable = voice.note == -1;
			if (voiceAvailable)
			{
				voice.channel = channelIdx + 2;
				return processNoteOn(voice, msg, ts);
			}
				
		}
		incChannelIdx();
		auto& voice = voices[channelIdx];
		voice.channel = channelIdx + 2;
		buffer.addEvent(MidiMessage::noteOff(voice.channel, voice.note), ts);
		processNoteOn(voice, msg, ts);
	}

	void AutoMPE::processNoteOn(Voice& voice, MidiMessage& msg, int ts) noexcept
	{
		const bool monophonic = poly == 1;
		if (monophonic)
			if(curNote != -1)
				buffer.addEvent(MidiMessage::noteOff(voice.channel, curNote), ts);
		const auto velo = msg.getVelocity();
		curNote = msg.getNoteNumber();
		heldNotes[curNote] = velo;
		if (velo == 0)
		{
			buffer.addEvent(MidiMessage::noteOff(voice.channel, curNote), ts);
			voice.note = curNote = -1;
			return;
		}
		voice.note = curNote;
		msg.setChannel(voice.channel);
		buffer.addEvent(msg, ts);
	}

	void AutoMPE::processNoteOff(MidiMessage& msg, int ts) noexcept
	{
		for (auto ch = 0; ch < poly; ++ch)
		{
			auto i = channelIdx - ch;
			if (i < 0)
				i += poly;
			auto& voice = voices[i];
			const auto nn = msg.getNoteNumber();
			heldNotes[nn] = 0;
			if (voice.note == nn)
				return processNoteOff(voice, msg, ts);
		}
	}

	void AutoMPE::processNoteOff(Voice& voice, MidiMessage& msg, int ts) noexcept
	{
		msg.setChannel(voice.channel);
		voice.note = curNote = -1;
		buffer.addEvent(msg, ts);
		const bool polyphonic = poly != 1;
		if (polyphonic)
			return;
		for (auto i = 0; i < 128; ++i)
			if (heldNotes[i] != 0)
			{
				voice.note = curNote = i;
				buffer.addEvent(MidiMessage::noteOn(voice.channel, curNote, heldNotes[i]), ts);
				return;
			}
	}

	void AutoMPE::processPitchWheel(MidiMessage& msg, int ts) noexcept
	{
		for (auto ch = 0; ch < poly; ++ch)
		{
			auto& voice = voices[ch];
			msg.setChannel(voice.channel);
			buffer.addEvent(msg, ts);
		}
	}
}