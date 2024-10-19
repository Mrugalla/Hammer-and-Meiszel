#pragma once
#include "../Using.h"
#include "../../arch/XenManager.h"
#include "../../arch/State.h"
#include <atomic>

namespace dsp
{
	struct KeySelector
	{
		static constexpr int NumKeys = PPDMaxXen;
		using XenManager = arch::XenManager;
		using State = arch::State;
		using String = arch::String;

		KeySelector() :
			keys(),
			requestUpdate(false),
			actives(),
			anchor(0),
			enabled(false)
		{
			bool sharps[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
			for (auto i = 0; i < 12; ++i)
				keys[i].store(sharps[i]);
			for (auto& active : actives)
				active = -1;
		}

		void loadPatch(const State& state)
		{
			bool e = false;
			for (auto i = 0; i < NumKeys; ++i)
			{
				const auto str = "keys/key" + String(i);
				const auto var = state.get(str);
				if (var)
				{
					const bool val = static_cast<int>(*var) == 1;
					keys[i].store(val);
					e = true;
				}
			}
			if(e)
				requestUpdate.store(true);
		}

		void savePatch(State& state) const
		{
			for (auto i = 0; i < NumKeys; ++i)
			{
				const auto str = "keys/key" + String(i);
				const auto val = keys[i].load() ? 1 : 0;
				state.set(str, val);
			}
		}

		void setKey(int keyIdx, bool e) noexcept
		{
			keys[keyIdx] = e;
			requestUpdate.store(true);
		}

		// midi, xen, enabled, playing
		void operator()(MidiBuffer& midi, const XenManager& xen,
			bool _enabled, bool)
		{
			const bool enabledSame = enabled == _enabled;
			if (enabledSame)
			{
				if (!enabled)
					return; // keyselector still disabled, thus return.
				// keyselector still enabled
				midi.clear();
				const bool updateRequested = requestUpdate.load();
				const auto _anchor = getAnchor(xen);
				const auto anchorChanged = anchor != _anchor;
				if (updateRequested || anchorChanged)
				{
					generateNoteOffs(midi);
					anchor = _anchor;
					updateActives();
					generateNoteOns(midi);
				}
			}
			else
			{
				enabled = _enabled;
				if (!enabled)
				{
					// it was enabled, but not anymore...
					midi.clear();
					generateNoteOffs(midi);
					enabled = false;
				}
				else
				{
					// it wasn't enabled, but now it is...
					midi.clear();
					midi.addEvent(MidiMessage::allNotesOff(1), 0);
					enabled = true;
					updateActives();
					generateNoteOns(midi);
				}
			}
		}

		std::array<std::atomic<bool>, NumKeys> keys;
		std::atomic<bool> requestUpdate;
	protected:
		std::array<int, NumMPEChannels> actives;
		int anchor;
		bool enabled;

		const int getAnchor(const XenManager& xen) const noexcept
		{
			return static_cast<int>(std::round(xen.getAnchor()));
		}

		void generateNoteOff(MidiBuffer& midi, int active)
		{
			const auto s = 0;
			const Uint8 velocity(127);
			const auto pitch = active + anchor;
			midi.addEvent(MidiMessage::noteOff(1, pitch, velocity), s);
		}

		void generateNoteOffs(MidiBuffer& midi)
		{
			for (auto i = 0; i < actives.size(); ++i)
			{
				const auto active = actives[i];
				if (active == -1)
					return;
				generateNoteOff(midi, active);
			}
		}

		void generateNoteOn(MidiBuffer& midi, int active)
		{
			const auto s = 0;
			const Uint8 velocity(127);
			const auto pitch = active + anchor;
			midi.addEvent(MidiMessage::noteOn(1, pitch, velocity), s);
		}

		void generateNoteOns(MidiBuffer& midi)
		{
			for (auto i = 0; i < actives.size(); ++i)
			{
				const auto active = actives[i];
				if (active == -1)
					return;
				generateNoteOn(midi, active);
			}
		}

		void updateActives() noexcept
		{
			auto aIdx = 0;
			for (auto i = 0; i < keys.size(); ++i)
			{
				const bool keyEnabled = keys[i].load();
				if (keyEnabled)
				{
					auto& active = actives[aIdx];
					active = i;
					++aIdx;
					if (aIdx == actives.size())
					{
						requestUpdate.store(false);
						return;
					}
				}
			}
			for (auto a = aIdx; a < actives.size(); ++a)
				actives[a] = -1;
			requestUpdate.store(false);
		}
	};
}