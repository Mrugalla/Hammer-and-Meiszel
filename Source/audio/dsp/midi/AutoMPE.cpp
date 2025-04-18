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
		poly(VoicesSize)
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
		channelIdx = -1;
		poly = _poly;
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
		const auto velo = msg.getVelocity();
		voice.note = msg.getNoteNumber();
		if (velo == 0)
		{
			buffer.addEvent(MidiMessage::noteOff(voice.channel, voice.note), ts);
			voice.note = -1;
			return;
		}
		msg.setChannel(voice.channel);
		buffer.addEvent(msg, ts);
	}

	void AutoMPE::processNoteOff(MidiMessage& msg, int ts) noexcept
	{
		for (auto ch = 0; ch < poly; ++ch)
		{
			auto i = channelIdx - ch;
			while (i < 0)
				i += poly;
			auto& voice = voices[i];
			const auto nn = msg.getNoteNumber();
			if (voice.note == nn)
				return processNoteOff(voice, msg, ts);
		}
	}

	void AutoMPE::processNoteOff(Voice& voice, MidiMessage& msg, int ts) noexcept
	{
		msg.setChannel(voice.channel);
		voice.note = -1;
		buffer.addEvent(msg, ts);
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