#include "TextEditor.h"

namespace gui
{
	TextEditor::TextEditor(Utils& u, const String& emptyString) :
			Button(u),
			labelEmpty(u),
			txt(""),
			txtEmpty(emptyString),
			caret(0),
			emptyAniIdx(0),
			active(true),
			onEnter([](){}),
			onKeyPress([](const KeyPress&) {})
		{
			addAndMakeVisible(labelEmpty);
			makeTextLabel(labelEmpty, emptyString, font::dosisMedium(), Just::centred, CID::Hover);
			labelEmpty.setInterceptsMouseClicks(false, false);

			makeTextButton(*this, "", "Pro tip: Use the keyboard to enter text!", CID::Txt);

			add(Callback([&]()
			{
				if (txt.isEmpty())
				{
					labelEmpty.setVisible(true);
					emptyAniIdx = (emptyAniIdx + 1) % 4;
					switch (emptyAniIdx)
					{
					case 0:
						labelEmpty.text = txtEmpty;
						break;
					case 1:
						labelEmpty.text = txtEmpty + ".";
						break;
					case 2:
						labelEmpty.text = txtEmpty + "..";
						break;
					case 3:
						labelEmpty.text = txtEmpty + "...";
						break;
					}
					labelEmpty.setMaxHeight();
					labelEmpty.repaint();
					return;
				}
				labelEmpty.setVisible(false);
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

			addEvt([&](evt::Type t, const void*)
			{
				if (t == evt::Type::DeactivateAllTextEditors)
				{
					active = false;
					giveAwayKeyboardFocus();
				}
			});

			setWantsKeyboardFocus(true);
		}

	bool TextEditor::keyPressed(const KeyPress& key)
	{
		if (!active)
			return false;
		if (key == KeyPress::returnKey)
		{
			txt = txt.substring(0, caret) + "\n" + txt.substring(caret);
			++caret;
			onEnter();
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
			if (caret > 0)
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
		onKeyPress(key);
		return true;
	}

	void TextEditor::resized()
	{
		Button::resized();
		const auto thicc = utils.thicc;
		labelEmpty.setBounds(getLocalBounds().toFloat().reduced(thicc).toNearestInt());
	}

	void TextEditor::clear()
	{
		setText("");
	}

	void TextEditor::setText(const String& nText)
	{
		txt = nText;
		caret = txt.length();
		updateLabel();
	}

	void TextEditor::addText(const String& nText)
	{
		txt = txt.substring(0, caret) + nText + txt.substring(caret);
		caret += nText.length();
		updateLabel();
	}

	void TextEditor::paste()
	{
		const auto cbTxt = SystemClipboard::getTextFromClipboard();
		if (cbTxt.isEmpty())
			return;
		addText(cbTxt);
	}

	void TextEditor::setActive(bool e)
	{
		notify(evt::Type::DeactivateAllTextEditors);
		active = e;
		updateActive();
	}

	void TextEditor::updateActive()
	{
		if (active)
			grabKeyboardFocus();
		else
			giveAwayKeyboardFocus();
	}

	bool TextEditor::isEmpty() const noexcept
	{
		return txt.isEmpty();
	}

	void TextEditor::updateLabel()
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
}