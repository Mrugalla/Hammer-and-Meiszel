#include "KeySelector.h"

namespace dsp
{
	KeySelector::KeySelector() :
		keys(),
		requestUpdate(true),
		actives(),
		offset(69 - 12),
		enabled(true)
	{
		bool sharps[12] = { false, true, false, false, false, true, true, false, false, false, true, false };
		for (auto i = 0; i < 12; ++i)
			keys[i].store(sharps[i]);
		for (auto& active : actives)
			active = -1;
	}

	void KeySelector::loadPatch(const State& state)
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
		if (e)
			requestUpdate.store(true);
	}

	void KeySelector::savePatch(State& state) const
	{
		for (auto i = 0; i < NumKeys; ++i)
		{
			const auto str = "keys/key" + String(i);
			const auto val = keys[i].load() ? 1 : 0;
			state.set(str, val);
		}
	}

	void KeySelector::prepare()
	{
		requestUpdate.store(true);
	}

	void KeySelector::setKey(int keyIdx, bool e) noexcept
	{
		keys[keyIdx] = e;
		requestUpdate.store(true);
	}

	int getOffset(const arch::XenManager& xen) noexcept
	{
		const auto oct = xen.getXen();
		const auto offset = 4. * oct;
		return static_cast<int>(std::round(offset));
	}

	// midi, xen, enabled, playing
	void KeySelector::operator()(MidiBuffer& midi, const XenManager& xen,
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
			const auto _offset = getOffset(xen);
			const auto offsetChanged = offset != _offset;
			if (updateRequested || offsetChanged)
			{
				generateNoteOffs(midi);
				offset = _offset;
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
				offset = getOffset(xen);
				updateActives();
				generateNoteOns(midi);
			}
		}
	}

	void KeySelector::generateNoteOff(MidiBuffer& midi, int active)
	{
		const auto s = 0;
		const Uint8 velocity(127);
		const auto pitch = active + offset;
		if(pitch >= 0 && pitch < 128)
			midi.addEvent(MidiMessage::noteOff(1, pitch, velocity), s);
	}

	void KeySelector::generateNoteOn(MidiBuffer& midi, int active)
	{
		const auto s = 0;
		const Uint8 velocity(127);
		const auto pitch = active + offset;
		if (pitch >= 0 && pitch < 128)
			midi.addEvent(MidiMessage::noteOn(1, pitch, velocity), s);
	}

	void KeySelector::generateNoteOffs(MidiBuffer& midi)
	{
		for (auto i = 0; i < actives.size(); ++i)
		{
			const auto active = actives[i];
			if (active == -1)
				return;
			generateNoteOff(midi, active);
		}
	}

	void KeySelector::generateNoteOns(MidiBuffer& midi)
	{
		for (auto i = 0; i < actives.size(); ++i)
		{
			const auto active = actives[i];
			if (active == -1)
				return;
			generateNoteOn(midi, active);
		}
	}

	void KeySelector::updateActives() noexcept
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
}