#pragma once
#include "Label.h"

namespace gui
{
	struct Tooltip :
		public Comp
	{
		Tooltip(Utils&);

		void resized() override;

	protected:
		enum { kTooltip, kBuildDate, kNumLabels };
		std::array<Label, kNumLabels> labels;
	};
}