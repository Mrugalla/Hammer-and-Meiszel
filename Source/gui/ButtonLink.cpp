#include "ButtonLink.h"

namespace gui
{
	ButtonLink::ButtonLink(Utils& u) :
		Button(u)
	{
	}

	void ButtonLink::init(const String& _name, const URL& url)
	{
		const auto col = getColour(CID::Interact).withRotatedHue(.5f);
		makeTextButton(*this, _name, "This button links to " + _name + "!", CID::Interact, col);
		onClick = [&](const Mouse&)
		{
			if (url.isWellFormed())
			{
				url.launchInDefaultBrowser();
				return;
			}
			const URL alt("https://theuselessweb.com/");
			alt.launchInDefaultBrowser();
		};
	}
}