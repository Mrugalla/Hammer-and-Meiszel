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
			caret(0),
			active(true)
		{
			addAndMakeVisible(labelEmpty);
			makeTextLabel(labelEmpty, "", font::dosisMedium(), Just::centred, CID::Hover);
			labelEmpty.setInterceptsMouseClicks(false, false);

			makeTextButton(*this, "",
				"Pro tip: Use the keyboard to enter text!", CID::Txt);

			add(Callback([&]()
			{
				if (active)
				{
					if (txt.isEmpty())
					{
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
					}
					else
					{
						return labelEmpty.setVisible(false);
					}
				}
			}, cbEmpty, cbFPS::k_1_875, true));

			add(Callback([&]()
			{
				if (isShowing() && active)
					grabKeyboardFocus();
			}, cbKeyFocus, cbFPS::k7_5, true));

			add(Callback([&]()
			{
				if (!active)
				{
					callbacks[cbCaret].phase = 0.f;
					updateLabel();
					return;
				}
				if (isShowing() && txt.isNotEmpty())
				{
					callbacks[cbCaret].phase = 1.f - callbacks[cbCaret].phase;
					updateLabel();
				}
			}, cbCaret, cbFPS::k_1_875, true));

			onClick = [&](const Mouse&)
			{
				setActive(true);
			};

			setWantsKeyboardFocus(true);
		}

		bool keyPressed(const KeyPress& key) override
		{
			if (!active)
				return false;
			if (key == KeyPress::returnKey)
			{
				txt = txt.substring(0, caret) + "\n" + txt.substring(caret);
				++caret;
			}
			else if (key == KeyPress::spaceKey)
			{
				txt = txt.substring(0, caret) + " " + txt.substring(caret);
				++caret;
			}
			else if (key == KeyPress::tabKey)
			{
				caret += 4;
				txt = txt.substring(0, caret) + "    " + txt.substring(caret);
			}
			else if (key == KeyPress::backspaceKey)
			{
				const auto c0 = caret - 1;
				if (txt.isNotEmpty() && c0 > -1)
				{
					txt = txt.substring(0, c0) + txt.substring(caret);
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
				if (isTextCharacter(wchar))
				{
					callbacks[cbCaret].start(1.f);
					txt = txt.substring(0, caret) + wchar + txt.substring(caret);
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

		void clear()
		{
			setText("");
		}

		void setText(const String& nText)
		{
			txt = nText;
			caret = txt.length();
			updateLabel();
		}

		void addText(const String& nText)
		{
			txt = txt.substring(0, caret) + nText + txt.substring(caret);
			caret += nText.length();
			updateLabel();
		}

		void paste()
		{
			const auto cbTxt = SystemClipboard::getTextFromClipboard();
			if (cbTxt.isEmpty())
				return;
			addText(cbTxt);
		}

		void setActive(bool e)
		{
			active = e;
			if (active)
				grabKeyboardFocus();
			else
				giveAwayKeyboardFocus();
		}

		bool isEmpty() const noexcept
		{
			return txt.isEmpty();
		}

		Label labelEmpty;
		String txt;
		int caret;
		bool active;

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
				label.text = txt.substring(0, caret) + " " + txt.substring(caret);
			else
				label.text = txt.substring(0, caret) + "|" + txt.substring(caret);
			label.setMaxHeight(utils.thicc);
			label.repaint();
		}
	};
}

/*

add ctrl+c ctrl+v ctrl+x functionality
make the caret have a different colour

*/