#pragma once
#include "Button.h"

namespace gui
{
	struct CompPower :
		public Comp
	{
		CompPower(Utils& u) :
			Comp(u)
		{
			setInterceptsMouseClicks(false, false);
			add(Callback([&]()
			{
				const auto val = utils.audioProcessor.params(PID::Power).getValMod();
				setVisible(val < .5f);
			}, 0, cbFPS::k7_5, true));
		}

		void paint(Graphics& g) override
		{
			setCol(g, CID::Darken);
			g.fillAll();
		}
	};

	struct ButtonPower :
		public Button
	{
		ButtonPower(Utils& u) :
			Button(u)
		{
			makeParameter(*this, PID::Power, Button::Type::kToggle, makeButtonOnPaintPower());
			type = Button::Type::kToggle;
			value = std::round(utils.audioProcessor.params(PID::Power).getValMod());
		}
	};
}