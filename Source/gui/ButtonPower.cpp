#include "ButtonPower.h"

namespace gui
{
	CompPower::CompPower(Utils& u) :
		Comp(u)
	{
		setInterceptsMouseClicks(false, false);
		add(Callback([&]()
			{
				const auto val = utils.audioProcessor.params(PID::Power).getValMod();
				setVisible(val < .5f);
			}, 0, cbFPS::k7_5, true));
	}

	void CompPower::paint(Graphics& g)
	{
		setCol(g, CID::Darken);
		g.fillAll();
	}

	//

	ButtonPower::ButtonPower(Utils& u) :
		Button(u)
	{
		makeParameter(*this, PID::Power, Button::Type::kToggle, makeButtonOnPaintPower());
		type = Button::Type::kToggle;
		value = std::round(utils.audioProcessor.params(PID::Power).getValMod());
	}
}