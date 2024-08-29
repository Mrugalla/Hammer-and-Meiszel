#include "Tooltip.h"

namespace gui
{
	String getBuildDate()
	{
		return static_cast<String>(__DATE__) + " " + static_cast<String>(__TIME__);
	}

	Tooltip::Tooltip(Utils& u) :
		Comp(u),
		labels
		{
			Label(u),
			Label(u)
		}
	{
		layout.init
		(
			{ 34, 1, 8 },
			{ 1 }
		);

		makeTextLabel(labels[kTooltip], "", font::dosisMedium(), Just::bottomLeft, CID::Txt, "Read the tooltips while hovering GUI elements to find out more about them!");

		const auto buildDateString = getBuildDate();
		const String buildDateTooltip = "This version's build date: " + buildDateString;
		makeTextLabel(labels[kBuildDate], buildDateString, font::dosisMedium(), Just::bottomRight, CID::Hover, buildDateTooltip);

		for (auto& label : labels)
			addAndMakeVisible(label);

		addEvt([&tt = labels[kTooltip]](evt::Type type, const void* stf)
		{
			if (type == evt::Type::TooltipUpdated)
			{
				const auto str = static_cast<const String*>(stf);
				tt.setText(str == nullptr ? "" : *str);
				tt.setMaxHeight();
				tt.repaint();
			}
		});
	}

	void Tooltip::paint(Graphics& g)
	{
		setCol(g, CID::Darken);
		g.fillAll();
	}

	void Tooltip::resized()
	{
		layout.resized(getLocalBounds());

		layout.place(labels[kTooltip], 0, 0, 1, 1, false);
		layout.place(labels[kBuildDate], 2, 0, 1, 1, false);

		for(auto& label: labels)
			label.setMaxHeight();
	}
}

//