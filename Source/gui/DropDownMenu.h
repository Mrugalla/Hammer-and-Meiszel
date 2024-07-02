#pragma once
#include "Button.h"

namespace gui
{
	struct DropDownMenu :
		public Comp
	{
		DropDownMenu(Utils& u) :
			Comp(u),
			buttons()
		{
			addEvt([&](evt::Type type, const void*)
			{
				if (type == evt::Type::ClickedEmpty)
					setVisible(false);
			});
		}

		void paint(Graphics& g) override
		{
			setCol(g, CID::Darken);
			const auto numButtons = buttons.size();
			for (auto b = 0; b < numButtons; ++b)
			{
				auto& btn = *buttons[b];
				const auto bounds = btn.getBounds().toFloat();
				g.fillRoundedRectangle(bounds, utils.thicc);
			}
		}

		void add(Button::OnPaint onPaint, Button::OnClick onClick)
		{
			buttons.push_back(std::make_unique<Button>(utils));
			auto& btn = *buttons.back().get();
			btn.onClick = onClick;
			btn.onPaint = onPaint;
		}

		void init()
		{
			for (auto& btn : buttons)
				addAndMakeVisible(*btn);
		}

		void resized() override
		{
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
			const auto numButtons = buttons.size();
			const auto numButtonsF = static_cast<float>(numButtons);
			const auto numButtonsInv = 1.f / numButtonsF;
			for (auto b = 0; b < numButtons; ++b)
			{
				const auto bF = static_cast<float>(b);
				const auto bR = bF * numButtonsInv;
				const auto x = 0.f;
				const auto y = bR * height;
				const auto w = width;
				const auto h = height * numButtonsInv;
				
				auto& btn = *buttons[b];
				btn.setBounds(BoundsF(x,y,w,h).reduced(utils.thicc).toNearestInt());
			}
		}

	private:
		std::vector<std::unique_ptr<Button>> buttons;
	};
}