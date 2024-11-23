#pragma once
#include "Button.h"
#include "../audio/dsp/KeySelector.h"

namespace gui
{
	struct KeySelectorEditor :
		public Comp
	{
		static constexpr int NumKeys = PPDMaxXen;

		enum kAnis { kXenUpdateCB, kKeysUpdateCB, kNumCallbacks };

		using KeySelector = dsp::KeySelector;

		KeySelectorEditor(Utils&, KeySelector&);

		void resized() override;
	
		KeySelector& selector;
		std::array<Button, NumKeys> keyButtons;
		Button keysEnabled;
		int numKeys;
	private:
		const int getXen() const noexcept;

		void initKeyButtons();
	};
}