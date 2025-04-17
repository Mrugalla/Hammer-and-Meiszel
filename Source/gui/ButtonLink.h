#pragma once
#include "Button.h"

namespace gui
{
	struct ButtonLink :
		public Button
	{
		struct Link { String name; URL url; };
		using Links = std::vector<Link>;

		ButtonLink(Utils&);

		// name, url
		void init(const String&, const URL&);
	};
}