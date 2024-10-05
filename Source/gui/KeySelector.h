#pragma once
#include "Button.h"
#include <array>

namespace gui
{
	struct KeySelector :
		public Comp
	{
		KeySelector(Utils& u) :
			Comp(u),
			keyButtons
			{
				Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u),
				Button(u), Button(u), Button(u), Button(u)
			},
			keysEnabled(u)
		{
			layout.init
			(
				{ 1, 8 },
				{ 1 }
			);

			addAndMakeVisible(keysEnabled);
			for (auto b = 0; b < keyButtons.size(); ++b)
			{
				auto& button = keyButtons[b];
				addAndMakeVisible(button);
				makeTextButton(button, String(b), "Click here to (de)activate this key.", CID::Interact);
			}
			makeParameter(keysEnabled, PID::KeySelectorEnabled, Button::Type::kToggle, "KEYS");
		}

		void resized() override
		{
			layout.resized(getLocalBounds().toFloat());
			layout.place(keysEnabled, 0, 0, 1, 1);
			const auto bounds = layout(1, 0, 1, 1);
			const auto w = bounds.getWidth() / keyButtons.size();
			const auto h = bounds.getHeight();
			const auto y = bounds.getY();
			auto x = bounds.getX();
			for (auto& button : keyButtons)
			{
				button.setBounds(BoundsF(x, y, w, h).toNearestInt());
				x += w;
			}
		}
	
		std::array<Button, 12> keyButtons;
		Button keysEnabled;
	};
}