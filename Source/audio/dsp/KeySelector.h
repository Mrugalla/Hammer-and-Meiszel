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

		KeySelector();

		// state
		void loadPatch(const State&);

		// state
		void savePatch(State&) const;

		void prepare();

		// keyIdx, e
		void setKey(int, bool) noexcept;

		// midi, xen, enabled, playing
		void operator()(MidiBuffer&, const XenManager&,
			bool, bool);

		std::array<std::atomic<bool>, NumKeys> keys;
		std::atomic<bool> requestUpdate;
	protected:
		std::array<int, NumMPEChannels> actives;
		int offset;
		bool enabled;

		// midi, active
		void generateNoteOff(MidiBuffer&, int);

		void generateNoteOffs(MidiBuffer&);

		// midi, active
		void generateNoteOn(MidiBuffer&, int active);

		void generateNoteOns(MidiBuffer&);

		void updateActives() noexcept;
	};
}