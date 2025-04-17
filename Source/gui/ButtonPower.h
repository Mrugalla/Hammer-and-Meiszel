#pragma once
#include "Button.h"

namespace gui
{
	struct CompPower :
		public Comp
	{
		CompPower(Utils&);

		void paint(Graphics&) override;
	};

	struct ButtonPower :
		public Button
	{
		ButtonPower(Utils&);
	};
}