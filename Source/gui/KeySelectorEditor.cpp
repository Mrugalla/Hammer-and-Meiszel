#include "KeySelectorEditor.h"

namespace gui
{
	KeySelectorEditor::KeySelectorEditor(Utils& u, KeySelector& _selector) :
		Comp(u),
		selector(_selector),
		keyButtons
		{
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u)
		},
		keysEnabled(u),
		numKeys(getXen())
	{
		layout.init
		(
			{ 1, 8 },
			{ 1 }
		);

		addAndMakeVisible(keysEnabled);
		for (auto& btn : keyButtons)
			addChildComponent(btn);
		makeParameter(keysEnabled, PID::KeySelectorEnabled, Button::Type::kToggle, "KEYS");

		initKeyButtons();

		add(Callback([&]()
		{
			const auto xen = getXen();
			if (numKeys == xen)
				return;
			numKeys = xen;
			for (auto i = 0; i < keyButtons.size(); ++i)
				keyButtons[i].setVisible(i < numKeys);
			resized();
		}, kAnis::kXenUpdateCB, cbFPS::k30, true));

		add(Callback([&]()
		{
			for (auto i = 0; i < numKeys; ++i)
			{
				const auto& key = selector.keys[i];
				auto& btn = keyButtons[i];

				const auto keyEnabled = key.load();
				const auto btnEnabled = btn.value > .5f;
				if (keyEnabled != btnEnabled)
				{
					btn.value = keyEnabled ? 1.f : 0.f;
					//btn.callbacks[Button::kToggleStateCB].start(0.f);
					btn.repaint();
				}
			}
		}, kAnis::kKeysUpdateCB, cbFPS::k15, true));
	}

	void KeySelectorEditor::resized()
	{
		layout.resized(getLocalBounds().toFloat());
		layout.place(keysEnabled, 0, 0, 1, 1);

		bool sharps[12] =
		{
			false, true, false, true, false, false, true, false, true, false, true, false
		};

		const auto bounds = layout(1, 0, 1, 1);
		const auto keysInv = 1.f / static_cast<float>(numKeys);
		const auto w = bounds.getWidth() * keysInv;
		const auto h = bounds.getHeight();
		const auto y = bounds.getY();
		auto x = bounds.getX();
		for (auto i = 0; i < numKeys; ++i)
		{
			auto& button = keyButtons[i];
			if (sharps[i % 12])
				button.setBounds(BoundsF(x, y, w, h * .8f).toNearestInt());
			else
				button.setBounds(BoundsF(x, y, w, h).toNearestInt());
			x += w;
		}
	}

	const int KeySelectorEditor::getXen() const noexcept
	{
		auto x = utils.audioProcessor.xenManager.getXen();
		return static_cast<int>(std::round(x));
	}

	void KeySelectorEditor::initKeyButtons()
	{
		String pitchclasses[12] =
		{
			"C", "#", "D", "#", "E", "F", "#", "G", "#", "A", "#", "B"
		};
		bool sharps[12] =
		{
			false, true, false, true, false, false, true, false, true, false, true, false
		};

		for (auto b = 0; b < keyButtons.size(); ++b)
		{
			auto& button = keyButtons[b];
			const auto oct = b / 12;
			const String octStr(oct == 0 ? "" : "\n" + String(oct));
			const auto b12 = b % 12;
			const String keyStr(pitchclasses[b12] + octStr);
			auto bgCol = getColour(CID::Bg);
			makeTextButton(button, keyStr, "Click here to (de)activate this key.", CID::Interact, bgCol);

			button.onClick = [&, b](const Mouse&)
			{
				auto& key = selector.keys[b];
				selector.setKey(b, !key.load());
			};
			button.type = Button::Type::kToggle;
		}

		for (auto i = 0; i < numKeys; ++i)
			keyButtons[i].setVisible(true);
	}
}