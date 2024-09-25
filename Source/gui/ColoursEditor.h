#pragma once
#include "Button.h"

namespace gui
{
	struct ColoursEditor :
		public Comp
	{
		static constexpr int NumColours = Colours::NumColours;

		ColoursEditor(Utils& u) :
			Comp(u),
			selector(27, 4, 7),
			buttonsColour
			{
				Button(u),
				Button(u),
				Button(u),
				Button(u),
				Button(u),
				Button(u)
			},
			buttonRevert(u),
			buttonDefault(u),
			cIdx(0),
			lastColour(Colours::c(cIdx))
		{
			setOpaque(true);

			layout.init
			(
				{ 1, 8 },
				{ 8, 1 }
			);

			addAndMakeVisible(selector);
			for (auto i = 0; i < NumColours; ++i)
			{
				auto& button = buttonsColour[i];
				addAndMakeVisible(button);

				const auto cID = static_cast<CID>(i);
				const auto name = toString(cID);

				makeTextButton(button, name, "Click here to change the colour of " + name + ".", CID::Interact);
				button.type = Button::Type::kChoice;
				button.value = 0.f;
				button.onClick = [&, i](const Mouse&)
				{
					cIdx = i;
					lastColour = Colours::c(cIdx);
					selector.setCurrentColour(Colours::c(cIdx));
					for (auto& button : buttonsColour)
						button.value = 0.f;
					buttonsColour[cIdx].value = 1.f;
				};
			}
			buttonsColour[cIdx].value = 1.f;
			selector.setCurrentColour(Colours::c(cIdx));

			addAndMakeVisible(buttonRevert);
			addAndMakeVisible(buttonDefault);

			makeTextButton(buttonRevert, "Revert", "Revert to the last saved colour settings.", CID::Interact);
			makeTextButton(buttonDefault, "Default", "Revert to the default colour settings.", CID::Interact);

			buttonRevert.onClick = [this](const Mouse&)
			{
				Colours::c.set(lastColour, static_cast<CID>(cIdx));
				selector.setCurrentColour(lastColour);
			};

			buttonDefault.onClick = [this](const Mouse&)
			{
				const auto cID = static_cast<CID>(cIdx);
				const auto col = toDefault(cID);
				Colours::c.set(col, cID);
				selector.setCurrentColour(col);
			};

			const auto fps = cbFPS::k7_5;
			const auto speed = msToInc(AniLengthMs, fps);
			add(Callback([&, speed]()
			{
				const auto selectorCol = selector.getCurrentColour();
				const auto curCol = Colours::c(cIdx);
				if (selectorCol == curCol)
					return;
				Colours::c.set(selectorCol, static_cast<CID>(cIdx));
				u.pluginTop.repaint();
			}, 0, fps, true));
		}

		void resized() override
		{
			layout.resized(getLocalBounds());

			const auto buttonsColourBounds = layout(0, 0, 1, 1);
			{
				const auto w = buttonsColourBounds.getWidth();
				const auto h = buttonsColourBounds.getHeight();
				const auto x = buttonsColourBounds.getX();
				const auto hButton = h / static_cast<float>(NumColours);
				auto y = buttonsColourBounds.getY();
				for (auto& button : buttonsColour)
				{
					const auto bounds = BoundsF(x, y, w, hButton);
					button.setBounds(bounds.toNearestInt());
					y += hButton;
				}
			}

			layout.place(selector, 1, 0, 1, 1);

			const auto buttonsBottomBounds = layout(1, 1, 1, 1);
			{
				const auto w = buttonsBottomBounds.getWidth();
				const auto wButton = w / 2.f;
				const auto h = buttonsBottomBounds.getHeight();
				const auto y = buttonsBottomBounds.getY();
				auto x = buttonsBottomBounds.getX();
				buttonRevert.setBounds(BoundsF(x, y, wButton, h).toNearestInt());
				x += wButton;
				buttonDefault.setBounds(BoundsF(x, y, wButton, h).toNearestInt());
			}
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xff000000));
		}

		ColourSelector selector;
		std::array<Button, NumColours> buttonsColour;
		Button buttonRevert, buttonDefault;
		int cIdx;
		Colour lastColour;
	};
}