#pragma once
#include "Button.h"

namespace gui
{
	struct ColoursEditor :
		public Comp
	{
		static constexpr int NumColours = Colours::NumColours;

		ColoursEditor(Utils&);

		void resized() override;

		void paint(Graphics&) override;

	private:
		std::unique_ptr<ColourSelector> selector;
		std::array<Button, NumColours> buttonsColour;
		Button buttonRevert, buttonDefault;
		int cIdx;
		Colour lastColour;

		void notifyUpdate(CID);
	};

	struct ButtonColours :
		public Button
	{
		ButtonColours(ColoursEditor&);

		void resized() override;
	private:
		Image img;
	};
}