#pragma once
#include "Button.h"

namespace gui
{
	struct ButtonLink :
		public Button
	{
		struct Link
		{
			String name;
			URL url;
		};

		using Links = std::vector<Link>;

		ButtonLink(Utils& u) :
			Button(u)
		{
		}

		void init(const String& _name, const URL& url)
		{
			const auto col = getColour(CID::Interact).withRotatedHue(.5f);
			makeTextButton(*this, _name, "This button links to " + _name + "!", CID::Interact, col);
			onClick = [&](const Mouse&)
			{
				url.launchInDefaultBrowser();
			};
		}
	};
}