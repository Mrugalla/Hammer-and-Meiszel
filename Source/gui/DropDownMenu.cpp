#include "DropDownMenu.h"

namespace gui
{
	DropDownMenu::DropDownMenu(Utils& u) :
		Comp(u),
		buttons()
	{
		addEvt([&](evt::Type type, const void*)
			{
				if (type == evt::Type::ClickedEmpty)
					setVisible(false);
			});
	}

	void DropDownMenu::paint(Graphics& g)
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

	void DropDownMenu::add(Button::OnPaint onPaint, Button::OnClick onClick)
	{
		buttons.push_back(std::make_unique<Button>(utils));
		auto& btn = *buttons.back().get();
		btn.onClick = onClick;
		btn.onPaint = onPaint;
	}

	void DropDownMenu::add(Button::OnClick onClick, const String& text, const String& _tooltip)
	{
		buttons.push_back(std::make_unique<Button>(utils));
		auto& btn = *buttons.back().get();
		makeTextButton(btn, text, _tooltip, CID::Interact);
		btn.onClick = onClick;
	}

	void DropDownMenu::init()
	{
		for (auto& btn : buttons)
			addAndMakeVisible(*btn);
	}

	void DropDownMenu::resized()
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
			btn.setBounds(BoundsF(x, y, w, h).reduced(utils.thicc).toNearestInt());
		}
	}
}