#include "MPESplit.h"

namespace dsp
{
	MPESplit::MPESplit() :
		buffers()
	{}

	void MPESplit::operator()(MidiBuffer& midiIn)
	{
		for (auto& buffer : buffers)
			buffer.clear();

		for (const auto midi : midiIn)
		{
			const auto msg = midi.getMessage();
			const auto ch = msg.getChannel();
			buffers[ch].addEvent(msg, midi.samplePosition);
		}

		midiIn.swapWith(buffers[Sysex]);
	}

	MidiBuffer& MPESplit::operator[](int ch) noexcept
	{
		return buffers[ch];
	}

	const MidiBuffer& MPESplit::operator[](int ch) const noexcept
	{
		return buffers[ch];
	}
}