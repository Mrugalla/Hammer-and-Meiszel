#pragma once
#include "../Using.h"
#include "../../arch/XenManager.h"
#include <atomic>

namespace dsp
{
	struct KeySelector
	{
		static constexpr int Octave = 3;
		static constexpr int NumKeys = PPDMaxXen;
		using XenManager = arch::XenManager;

		KeySelector() :
			keys(),
			buffer(),
			actives(),
			playing(false)
		{
			bool sharps[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
			for (auto i = 0; i < NumKeys; ++i)
				keys[i].store(sharps[i % 12]);
			for(auto& active : actives)
				active = false;
		}

		void operator()(MidiBuffer& midi, const XenManager& xen,
			bool useKeys, bool _playing)
		{
			if (!useKeys)
				return;
			/*
			gateNoteOffs(midi);
			const auto xenVal = static_cast<int>(std::round(xen.getXen()));
			if (playbackStopped(_playing))
				for (auto i = 0; i < NumKeys; ++i)
					if (actives[i])
						makeNoteOffs(midi, i, xenVal);
			updateKeySelector(midi, xenVal);
			*/
		}

		std::array<std::atomic<bool>, NumKeys> keys;
	protected:
		MidiBuffer buffer;
		std::array<bool, NumKeys> actives;
		bool playing;

		void gateNoteOffs(MidiBuffer& midi)
		{
			buffer.clear();
			for (const auto& it : midi)
			{
				const auto msg = it.getMessage();
				if (msg.isNoteOff() || msg.isAllNotesOff())
					buffer.addEvent(msg, 0);
			}
			midi.swapWith(buffer);
		}

		bool playbackStopped(bool _playing) noexcept
		{
			if (playing && !_playing)
			{
				playing = _playing;
				return true;
			}
			playing = _playing;
			return false;
		}

		void makeNoteOffs(MidiBuffer& midi, int i, int xenVal)
		{
			for (auto j = 0; j < 3; ++j)
				midi.addEvent(MidiMessage::noteOff(1, (1 << Octave) + i + j * xenVal), 0);
			actives[i] = false;
		}

		void makeNoteOns(MidiBuffer& midi, int i, int xenVal)
		{
			for (auto j = 0; j < 3; ++j)
				midi.addEvent(MidiMessage::noteOn(1, (1 << Octave) + i + j * xenVal, 1.f), 0);
			actives[i] = true;
		}

		void updateKeySelector(MidiBuffer& midi, int xenVal)
		{
			for (auto i = 0; i < NumKeys; ++i)
			{
				const auto key = keys[i].load();
				const auto active = actives[i];
				if (key && !active)
					makeNoteOns(midi, i, xenVal);
				else if (!key && active)
					makeNoteOffs(midi, i, xenVal);
			}
		}
	};
}