#pragma once
#include "Button.h"

namespace gui
{
	struct TextEditor :
		public Button
	{
		enum { cbEmpty, cbKeyFocus, cbCaret };

		// u, emptyString
		TextEditor(Utils&, const String & = "");

		bool keyPressed(const KeyPress&) override;

		void resized() override;

		void clear();

		void setText(const String&);

		void addText(const String&);

		void paste();

		virtual void setActive(bool e);

		void updateActive();

		bool isEmpty() const noexcept;

		Label labelEmpty;
		String txt, txtEmpty;
		int caret, emptyAniIdx;
		bool active;
		std::function<void()> onEnter;
		std::function<void(const KeyPress&)> onKeyPress;
	private:
		void updateLabel();
	};
}

/*
add ctrl+c ctrl+v ctrl+x functionality
make the caret have a different colour
*/