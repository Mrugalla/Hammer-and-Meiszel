#pragma once
#include "../../Using.h"

namespace dsp
{
	struct MidiTranspose
	{
		MidiTranspose() :
			buffer(),
			retuneSemi(0)
		{}

		void operator()(MidiBuffer& midi, int _retuneSemi)
		{
			buffer.clear();

			if (retuneSemi != _retuneSemi)
			{
				retuneSemi = _retuneSemi;
				for (auto i = 1; i <= NumMIDIChannels; ++i)
				{
					const auto msg = MidiMessage::allNotesOff(i);
					buffer.addEvent(msg, 0);
				}
			}

			for (const auto it : midi)
			{
				auto msg = it.getMessage();
				if (msg.isNoteOnOrOff())
				{
					const auto pitch = juce::jlimit(0, 127, msg.getNoteNumber() + retuneSemi);
					msg.setNoteNumber(pitch);
				}
				buffer.addEvent(msg, it.samplePosition);
			}

			midi.swapWith(buffer);
		}

	protected:
		MidiBuffer buffer;
		int retuneSemi;
	};
}