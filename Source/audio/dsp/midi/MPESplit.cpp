#include "MPESplit.h"

namespace dsp
{
	MPESplit::MPESplit() :
		buffers()
	{}

	void MPESplit::operator()(MidiBuffer& midiIn, int numSamples)
	{
		for (auto& buffer : buffers)
			buffer.clear();

		for (const auto midi : midiIn)
		{
			const auto msg = midi.getMessage();
			const auto ch = msg.getChannel();
			buffers[ch].addEvent(msg, midi.samplePosition);
		}
		for (auto i = 1; i < Size; ++i)
		{
			const auto msg = MidiMessage::controllerEvent(i, 69, 69);
			buffers[i].addEvent(msg, numSamples);
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