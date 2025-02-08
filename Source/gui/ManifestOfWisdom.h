#pragma once
#include "TextEditor.h"

namespace gui
{
	struct ManifestOfWisdom :
		public Comp
	{
		ManifestOfWisdom(Utils&);

		void paint(Graphics&) override;

		void resized() override;

	private:
		Label title, subTitle, alert;
		TextEditor editor;
		Button manifest, inspire, reveal, clear, paste;
		LabelGroup buttonLabelsGroup;

		// msg
		void parse(const String& = "");
	};

	struct ButtonWisdom :
		public Button
	{
		ButtonWisdom(Utils&, ManifestOfWisdom&);

		void resized() override;

	protected:
		Image book, bookHover;
		int bookX, bookY;
	};
}