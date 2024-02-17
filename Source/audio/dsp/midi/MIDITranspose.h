#pragma once
#include "../../Using.h"

namespace dsp
{
	struct MidiTranspose
	{
		struct Note
		{
			Note() :
				pitch(0),
				channel(1)
			{}

			int pitch, channel;
		};

		using Notes = std::array<Note, NumMIDIChannels>;

		MidiTranspose() :
			notes(),
			buffer(),
			retuneSemi(0),
			idx(0)
		{}

		void operator()(MidiBuffer& midi, int _retuneSemi)
		{
			buffer.clear();

			if (retuneSemi != _retuneSemi)
			{
				retuneSemi = _retuneSemi;
				for (auto& note: notes)
				{
					const auto msg = MidiMessage::noteOff(note.channel, note.pitch);
					buffer.addEvent(msg, 0);
				}
			}
			else
				for (const auto it : midi)
				{
					auto msg = it.getMessage();
					if (msg.isNoteOnOrOff())
					{
						if(msg.isNoteOn())
							idx = (idx + 1) & NumMPEChannels;
						auto& note = notes[idx];
						note.pitch = juce::jlimit(0, 127, msg.getNoteNumber() + retuneSemi);
						msg.setNoteNumber(note.pitch);
					}
					buffer.addEvent(msg, it.samplePosition);
				}

			midi.swapWith(buffer);
		}

	protected:
		Notes notes;
		MidiBuffer buffer;
		int retuneSemi, idx;
	};
}