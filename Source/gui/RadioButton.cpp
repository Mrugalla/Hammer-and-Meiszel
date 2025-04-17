#include "RadioButton.h"

namespace gui
{
	RadioButton::RadioButton(Utils& u) :
		Comp(u),
		buttons(),
		labelGroup()
	{
	}

	void RadioButton::clear()
	{
		for (auto& button : buttons)
			removeChildComponent(button.get());
		buttons.clear();
		labelGroup.clear();
	}

	void RadioButton::attach(PID pID)
	{
		clear();
		auto& param = utils.getParam(pID);
		const auto& range = param.range;
		const auto numSteps = static_cast<int>(range.end - range.start) + 1;
		for (auto i = 0; i < numSteps; ++i)
			buttons.push_back(std::make_unique<Button>(utils));
		makeParameter(buttons, PID::ModSelect);
		for (auto& button : buttons)
		{
			auto& btn = *button;
			addAndMakeVisible(btn);
			btn.label.autoMaxHeight = false;
			labelGroup.add(btn.label);
		}
	}

	void RadioButton::resized()
	{
		const auto numButtons = buttons.size();
		const auto w = static_cast<float>(getWidth());
		const auto h = static_cast<float>(getHeight());
		const auto thicc = utils.thicc;
		const auto buttonW = w / static_cast<float>(numButtons);
		const auto y = 0.f;
		auto x = 0.f;
		for (auto& button : buttons)
		{
			button->setBounds(BoundsF(x, y, buttonW, h).toNearestInt());
			x += buttonW;
		}
		labelGroup.setMaxHeight(thicc);
	}
}