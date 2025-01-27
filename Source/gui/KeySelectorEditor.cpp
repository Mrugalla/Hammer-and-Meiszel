#include "KeySelectorEditor.h"

namespace gui
{
	KeySelectorEditor::KeySelectorEditor(Utils& u, KeySelector& _selector) :
		Comp(u),
		selector(_selector),
		keyButtons
		{
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u),
			Button(u), Button(u), Button(u), Button(u), Button(u), Button(u)
		},
		keysEnabled(u),
		numKeys(getXen())
	{
		layout.init
		(
			{ 1, 8 },
			{ 1 }
		);

		addAndMakeVisible(keysEnabled);
		for (auto& btn : keyButtons)
			addChildComponent(btn);
		makeParameter(keysEnabled, PID::KeySelectorEnabled, Button::Type::kToggle, [](Graphics& g, const Button& b)
			{
				const auto thicc = b.utils.thicc;
				const auto thiccHalf = thicc * .5f;
				const auto thicc2 = thicc * 2.f;
				const auto thicc3 = thicc * 3.f;
				const auto thicc5 = thicc * 5.f;

				const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
				const auto togglePhase = b.callbacks[Button::kToggleStateCB].phase;

				const auto lineThicc = thicc;
				const auto blackKeyThicc = thicc3 - hoverPhase * thicc;

				const auto imgHeightRel = .5f;

				auto bounds = b.getLocalBounds().toFloat().reduced(lineThicc);
				auto imgBounds = bounds.withHeight(bounds.getHeight() * imgHeightRel);

				setCol(g, CID::Bg);
				g.fillRoundedRectangle(bounds, lineThicc);
				setCol(g, CID::Interact);

				// draw keys image
				{
					const auto x = imgBounds.getX();
					const auto y = imgBounds.getY();
					const auto w = imgBounds.getWidth();
					const auto h = imgBounds.getHeight();

					const auto btm0 = y + h;
					const auto btm1 = y + h * .5f;
					bool blackKeyMask[8] = { false, true, true, false, true, true, true, false };

					const auto numKeysPerOct = 4.f + hoverPhase * (8.f - 4.f);
					const auto maxKeysPerOct = numKeysPerOct - 1.f;
					const auto wKey = w / maxKeysPerOct;
					const auto maxKeysPerOctInt = static_cast<int>(maxKeysPerOct);
					for (auto i = 1; i < maxKeysPerOctInt; ++i)
					{
						const auto xKey = x + i * wKey;
						g.drawLine(xKey, y, xKey, btm0, lineThicc);
						if (blackKeyMask[i])
							g.drawLine(xKey, y, xKey, btm1, blackKeyThicc);
					}
				}

				// draw arrow
				if (togglePhase > 0.f)
				{
					const auto x0 = imgBounds.getX();
					const auto x1 = imgBounds.getRight() * togglePhase * togglePhase;
					const auto y = imgBounds.getBottom() + .5f * (bounds.getHeight() - imgBounds.getHeight());
					const LineF arrow(x0, y, x1, y);
					g.drawArrow(arrow, lineThicc, thicc3, thicc3);
				}

				// draw MIDI string
				if (togglePhase < .5f)
				{
					const auto tp2 = 1.f - togglePhase * 2.f;
					const String midiStr("MIDI");
					const auto strLen = static_cast<float>(midiStr.length());
					const auto substrLen = static_cast<int>(strLen * tp2);
					const String substr = midiStr.substring(0, substrLen);

					const auto x = bounds.getX();
					const auto y = imgBounds.getBottom();
					const auto w = bounds.getWidth();
					const auto h = bounds.getHeight() - y;
					const BoundsF textBounds(x, y, w, h);
					const auto tFont = font::dosisBold();
					g.setFont(tFont);
					const auto height = findMaxHeight(tFont, substr, w, h);
					g.setFont(height);
					g.drawFittedText(midiStr.substring(0, substrLen), textBounds.toNearestInt(), Just::centred, 1);
				}
			});

		initKeyButtons();

		add(Callback([&]()
		{
			const auto xen = getXen();
			if (numKeys == xen)
				return;
			numKeys = xen;
			for (auto i = 0; i < keyButtons.size(); ++i)
				keyButtons[i].setVisible(i < numKeys);
			resized();
		}, kAnis::kXenUpdateCB, cbFPS::k30, true));

		add(Callback([&]()
		{
			for (auto i = 0; i < numKeys; ++i)
			{
				const auto& key = selector.keys[i];
				auto& btn = keyButtons[i];

				const auto keyEnabled = key.load();
				const auto btnEnabled = btn.value > .5f;
				if (keyEnabled != btnEnabled)
				{
					btn.value = keyEnabled ? 1.f : 0.f;
					btn.repaint();
				}
			}
		}, kAnis::kKeysUpdateCB, cbFPS::k15, true));
	}

	void KeySelectorEditor::resized()
	{
		layout.resized(getLocalBounds().toFloat());
		layout.place(keysEnabled, 0, 0, 1, 1);

		bool sharps[12] =
		{
			false, true, false, true, false, false, true, false, true, false, true, false
		};

		const auto bounds = layout(1, 0, 1, 1);
		const auto keysInv = 1.f / static_cast<float>(numKeys);
		const auto w = bounds.getWidth() * keysInv;
		const auto h = bounds.getHeight();
		const auto y = bounds.getY();
		auto x = bounds.getX();
		for (auto i = 0; i < numKeys; ++i)
		{
			auto& button = keyButtons[i];
			if (sharps[i % 12])
				button.setBounds(BoundsF(x, y, w, h * .8f).toNearestInt());
			else
				button.setBounds(BoundsF(x, y, w, h).toNearestInt());
			x += w;
		}
	}

	const int KeySelectorEditor::getXen() const noexcept
	{
		auto x = utils.audioProcessor.xenManager.getXen();
		return static_cast<int>(std::round(x));
	}

	void KeySelectorEditor::initKeyButtons()
	{
		String pitchclasses[12] =
		{
			"C", "#", "D", "#", "E", "F", "#", "G", "#", "A", "#", "B"
		};
		bool sharps[12] =
		{
			false, true, false, true, false, false, true, false, true, false, true, false
		};

		for (auto b = 0; b < keyButtons.size(); ++b)
		{
			auto& button = keyButtons[b];
			const auto oct = b / 12;
			const String octStr(oct == 0 ? "" : "\n" + String(oct));
			const auto b12 = b % 12;
			const String keyStr(pitchclasses[b12] + octStr);
			auto bgCol = getColour(CID::Bg);
			makeTextButton(button, keyStr, "Click here to (de)activate this key.", CID::Interact, bgCol);

			button.onClick = [&, b](const Mouse&)
			{
				auto& key = selector.keys[b];
				selector.setKey(b, !key.load());
			};
			button.type = Button::Type::kToggle;
		}

		for (auto i = 0; i < numKeys; ++i)
			keyButtons[i].setVisible(true);
	}
}