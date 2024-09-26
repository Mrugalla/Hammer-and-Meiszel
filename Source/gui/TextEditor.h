#pragma once
#include "Button.h"

namespace gui
{
	struct TextEditor :
		public Button
	{
		enum { cbEmpty, cbKeyFocus, cbCaret };

		TextEditor(Utils& u) :
			Button(u),
			labelEmpty(u),
			txt(""),
			caret(0)
		{
			addAndMakeVisible(labelEmpty);
			makeTextLabel(labelEmpty, "", font::dosisMedium(), Just::centred, CID::Hover);
			labelEmpty.setInterceptsMouseClicks(false, false);

			makeTextButton(*this, "",
				"Pro tip: Use the keyboard to enter text!", CID::Txt);

			add(Callback([&]()
			{
				if (txt.isNotEmpty())
					return labelEmpty.setVisible(false);
				labelEmpty.setVisible(true);
				if (labelEmpty.text == ".")
					labelEmpty.text = "..";
				else if (labelEmpty.text == "..")
					labelEmpty.text = "...";
				else if (labelEmpty.text == "...")
					labelEmpty.text = "";
				else
					labelEmpty.text = ".";
				labelEmpty.setMaxHeight();
				labelEmpty.repaint();

			}, cbEmpty, cbFPS::k_1_875, true));

			add(Callback([&]()
			{
				if (isShowing())
					grabKeyboardFocus();

			}, cbKeyFocus, cbFPS::k7_5, true));

			add(Callback([&]()
			{
				if (isShowing() && txt.isNotEmpty())
				{
					callbacks[cbCaret].phase = 1.f - callbacks[cbCaret].phase;
					updateLabel();
				}
			}, cbCaret, cbFPS::k_1_875, true));

			onClick = [&](const Mouse&)
			{
				// manifest
			};

			setWantsKeyboardFocus(true);
		}

		bool keyPressed(const KeyPress& key) override
		{
			if (key == KeyPress::backspaceKey)
			{
				if (txt.isNotEmpty())
				{
					txt = txt.substring(0, caret - 1) + txt.substring(caret);
					--caret;
				}
			}
			else if (key == KeyPress::deleteKey)
			{
				if (caret < txt.length())
					txt = txt.substring(0, caret) + txt.substring(caret + 1);
			}
			else if (key == KeyPress::leftKey)
			{
				if(caret > 0)
					--caret;
			}
			else if (key == KeyPress::rightKey)
			{
				if (caret < txt.length())
					++caret;
			}
			else
			{
				const auto wchar = key.getTextCharacter();
				if (wchar > 'a' && wchar < 'z' || wchar > 'A' && wchar < 'Z' || wchar > '0' && wchar < '9' || wchar == ' ' || wchar == '.' || wchar == ',' || wchar == '!' || wchar == '?' || wchar == ':' || wchar == ';' || wchar == '(' || wchar == ')' || wchar == '[' || wchar == ']' || wchar == '{' || wchar == '}' || wchar == '-' || wchar == '_' || wchar == '+' || wchar == '=' || wchar == '*' || wchar == '/' || wchar == '\\' || wchar == '|' || wchar == '<' || wchar == '>' || wchar == '@' || wchar == '#' || wchar == '$' || wchar == '%' || wchar == '^' || wchar == '&' || wchar == '*' || wchar == '~' || wchar == '`' || wchar == '"' || wchar == '\'' || wchar == ' ')
				{
					callbacks[cbCaret].start(1.f);
					txt += wchar;
					++caret;
				}
			}

			updateLabel();
			return true;
		}

		void resized() override
		{
			Button::resized();
			const auto thicc = utils.thicc;
			labelEmpty.setBounds(getLocalBounds().toFloat().reduced(thicc * 5.f).toNearestInt());
		}

		Label labelEmpty;
		String txt;
		int caret;

	private:
		void updateLabel()
		{
			if (txt.isEmpty())
			{
				label.text = "";
				label.repaint();
				labelEmpty.setVisible(true);
				return;
			}

			labelEmpty.setVisible(false);
			if (callbacks[cbCaret].phase < .5f)
				label.text = txt;
			else
				label.text = txt.substring(0, caret) + "|" + txt.substring(caret);
			label.setMaxHeight();
			label.repaint();
		}
	};
}

/*

add ctrl+c ctrl+v ctrl+x functionality
make the caret have a different colour

*/