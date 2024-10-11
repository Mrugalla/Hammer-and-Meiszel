#pragma once
#include "Button.h"
#include <array>

namespace gui
{
	struct KeySelector :
		public Comp
	{
		static constexpr int NumKeys = PPDMaxXen;

		KeySelector(Utils& u) :
			Comp(u),
			keyButtons
			{
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u), Button(u)
			},
			keysEnabled(u),
			numKeys(getXen())
		{
			layout.init
			(
				{ 1, 8 },
				{ 1 }
			);

			String pitchclasses[12] =
			{
				"C", "C\n#", "D", "D\n#", "E", "F", "F\n#", "G", "G\n#", "A", "A\n#", "B"
			};

			addAndMakeVisible(keysEnabled);
			for (auto b = 0; b < keyButtons.size(); ++b)
			{
				auto& button = keyButtons[b];
				addChildComponent(button);
				const auto oct = b / 12;
				const String octStr(oct == 0 ? "" : "\n" + String(oct));
				const String keyStr(pitchclasses[b % 12] + octStr);
				makeTextButton(button, keyStr, "Click here to (de)activate this key.", CID::Interact);
			}
			makeParameter(keysEnabled, PID::KeySelectorEnabled, Button::Type::kToggle, "KEYS");

			for (auto i = 0; i < numKeys; ++i)
				keyButtons[i].setVisible(true);

			add(Callback([&]()
			{
				const auto xen = getXen();
				if (numKeys == xen)
					return;

				numKeys = xen;
				for (auto i = 0; i < keyButtons.size(); ++i)
					keyButtons[i].setVisible(i < numKeys);
				resized();
			}, 0, cbFPS::k30, true));
		}

		void resized() override
		{
			layout.resized(getLocalBounds().toFloat());
			layout.place(keysEnabled, 0, 0, 1, 1);
			const auto bounds = layout(1, 0, 1, 1);
			const auto keysInv = 1.f / static_cast<float>(numKeys);
			const auto w = bounds.getWidth() * keysInv;
			const auto h = bounds.getHeight();
			const auto y = bounds.getY();
			auto x = bounds.getX();
			for (auto i = 0; i < numKeys; ++i)
			{
				auto& button = keyButtons[i];
				button.setBounds(BoundsF(x, y, w, h).toNearestInt());
				x += w;
			}
		}
	
		std::array<Button, NumKeys> keyButtons;
		Button keysEnabled;
		int numKeys;

	private:
		const int getXen() const noexcept
		{
			auto x = utils.audioProcessor.xenManager.getXen();
			return static_cast<int>(std::round(x));
		}
	};
}