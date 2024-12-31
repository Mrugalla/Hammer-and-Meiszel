#pragma once
#include "Button.h"

namespace gui
{
	struct DropDownMenu :
		public Comp
	{
		DropDownMenu(Utils&);

		void paint(Graphics&) override;

		void add(Button::OnPaint, Button::OnClick);

		// onClick, text, tooltip
		void add(Button::OnClick, const String&, const String&);

		void init();

		void resized() override;

	private:
		std::vector<std::unique_ptr<Button>> buttons;
	};

	struct ButtonDropDown :
		public Button
	{
		ButtonDropDown(Utils& u) :
			Button(u)
		{}

		void init(DropDownMenu& dropDown, const String& title, const String& _tooltip)
		{
			makeTextButton(*this, title, _tooltip, CID::Interact);
			type = Button::Type::kToggle;
			onPaint = makeButtonOnPaint(true, getColour(CID::Bg));
			onClick = [&m = dropDown](const Mouse&)
			{
				auto e = !m.isVisible();
				m.notify(evt::Type::ClickedEmpty);
				m.setVisible(e);
			};
			add(Callback([&]()
			{
				const auto v = value > .5f;
				const auto e = dropDown.isVisible();
				if (v == e)
					return;
				value = e ? 1.f : 0.f;
				repaint();
			}, 3, cbFPS::k15, true));
		}
	};
}