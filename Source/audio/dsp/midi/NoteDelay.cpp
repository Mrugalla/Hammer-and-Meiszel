#include "NoteDelay.h"

namespace dsp
{

	SampleDelay::SampleDelay() :
		buffer(),
		temp(),
		needTemp(false)
	{}

	void SampleDelay::operator()(MidiBuffer& midi, int numSamples)
	{
		buffer.clear();
		if (needTemp)
		{
			needTemp = false;
			buffer.addEvent(temp, 0);
		}

		for (const auto it : midi)
		{
			const auto msg = it.getMessage();
			const auto ts = it.samplePosition;
			if (msg.isNoteOnOrOff())
			{
				if (ts < numSamples)
					buffer.addEvent(msg, ts + 1);
				else
					temp = msg;
			}
			else
				buffer.addEvent(msg, ts);
		}

		midi.swapWith(buffer);
	}

	Delay::Msg::Msg() :
		msg(),
		ts(-1)
	{}

	Delay::Delay() :
		buffer(),
		stack(),
		idx(0)
	{
	}

	void Delay::operator()(MidiBuffer& midi,
		int numSamples, int delayLength)
	{
		seperateNotesFromRest(midi, delayLength);
		processDelayedNotes(midi, numSamples);
	}

	void Delay::seperateNotesFromRest(MidiBuffer& midi, int delayLength)
	{
		buffer.clear();

		for (const auto it : midi)
		{
			const auto msg = it.getMessage();
			const auto ts = it.samplePosition;
			if (msg.isNoteOnOrOff())
				addToStack(msg, ts + delayLength);
			else
				buffer.addEvent(msg, ts);
		}

		midi.swapWith(buffer);
	}

	void Delay::addToStack(const MidiMessage& msg, int nTs) noexcept
	{
		for (auto i = 0; i < Size; ++i)
		{
			idx = (idx + 1) & Max;
			if (stack[idx].ts == -1)
			{
				stack[idx].msg = msg;
				stack[idx].ts = nTs;
				return;
			}
		}
		idx = (idx + 1) & Max;
		stack[idx].msg = msg;
		stack[idx].ts = nTs;
	}

	void Delay::processDelayedNotes(MidiBuffer& midi, int numSamples)
	{
		buffer.clear();

		for (auto i = 0; i < Size; ++i)
		{
			const auto i2 = (idx + i + 1) & Max;
			auto& msg = stack[i2];
			if (msg.ts != -1)
			{
				if (msg.ts < numSamples)
				{
					buffer.addEvent(msg.msg, msg.ts);
					msg.ts = -1;
				}
				else
				{
					msg.ts -= numSamples;
				}
			}
		}

		midi.addEvents(buffer, 0, numSamples, 0);
	}
}