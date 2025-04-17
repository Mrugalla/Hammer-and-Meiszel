#pragma once
#include "Button.h"

namespace gui
{
	struct RadioButton :
		public Comp
	{
		RadioButton(Utils&);

		void clear();

		void attach(PID);

		void resized() override;
	private:
		std::vector<std::unique_ptr<Button>> buttons;
		LabelGroup labelGroup;
	};
}