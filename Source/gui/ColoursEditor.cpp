#include "ColoursEditor.h"

namespace gui
{
	ColoursEditor::ColoursEditor(Utils& u) :
		Comp(u),
		selector(std::make_unique<ColourSelector>(26, 4, 7)),
		buttonsColour
		{
			Button(u),
			Button(u),
			Button(u),
			Button(u),
			Button(u),
			Button(u)
		},
		buttonRevert(u),
		buttonDefault(u),
		cIdx(0),
		lastColour(Colours::c(cIdx))
	{
		setOpaque(true);
		selector->setMouseCursor(makeCursor());

		layout.init
		(
			{ 1, 8 },
			{ 8, 1 }
		);

		addAndMakeVisible(*selector);
		for (auto i = 0; i < NumColours; ++i)
		{
			auto& button = buttonsColour[i];
			addAndMakeVisible(button);

			const auto cID = static_cast<CID>(i);
			const auto name = toString(cID);

			makeTextButton(button, name, "Click here to change the colour of " + name + ".", CID::Interact);
			button.type = Button::Type::kChoice;
			button.value = 0.f;
			button.onClick = [&, i](const Mouse&)
			{
				cIdx = i;
				lastColour = Colours::c(cIdx);
				removeChildComponent(selector.get());
				const auto cID = static_cast<CID>(cIdx);
				switch (cID)
				{
				case CID::Bg:
				case CID::Txt:
				case CID::Interact:
				case CID::Mod:
					selector = std::make_unique<ColourSelector>(26, 4, 7);
					break;
				case CID::Hover:
				case CID::Darken:
					selector = std::make_unique<ColourSelector>(27, 4, 7);
					break;
				}
				addAndMakeVisible(*selector);
				selector->setCurrentColour(Colours::c(cIdx));
				for (auto& button : buttonsColour)
					button.value = 0.f;
				buttonsColour[cIdx].value = 1.f;
				resized();
			};
		}
		buttonsColour[cIdx].value = 1.f;
		selector->setCurrentColour(Colours::c(cIdx));

		addAndMakeVisible(buttonRevert);
		addAndMakeVisible(buttonDefault);

		makeTextButton(buttonRevert, "Revert", "Revert to the last saved colour settings.", CID::Interact);
		makeTextButton(buttonDefault, "Default", "Revert to the default colour settings.", CID::Interact);

		buttonRevert.onClick = [&](const Mouse&)
		{
			const auto cID = static_cast<CID>(cIdx);
			Colours::c.set(lastColour, cID);
			selector->setCurrentColour(lastColour);
			utils.pluginTop.repaint();
			updateCursor(cID);
		};

		buttonDefault.onClick = [&](const Mouse&)
		{
			const auto cID = static_cast<CID>(cIdx);
			const auto col = toDefault(cID);
			Colours::c.set(col, cID);
			selector->setCurrentColour(col);
			utils.pluginTop.repaint();
			updateCursor(cID);
		};

		const auto fps = cbFPS::k7_5;
		const auto speed = msToInc(AniLengthMs, fps);
		add(Callback([&, speed]()
		{
			const auto selectorCol = selector->getCurrentColour();
			const auto curCol = Colours::c(cIdx);
			if (selectorCol == curCol)
				return;
			const auto cID = static_cast<CID>(cIdx);
			Colours::c.set(selectorCol, cID);
			u.pluginTop.repaint();
			updateCursor(cID);
		}, 0, fps, true));
	}

	void ColoursEditor::resized()
	{
		layout.resized(getLocalBounds());

		const auto buttonsColourBounds = layout(0, 0, 1, 1);
		{
			const auto w = buttonsColourBounds.getWidth();
			const auto h = buttonsColourBounds.getHeight();
			const auto x = buttonsColourBounds.getX();
			const auto hButton = h / static_cast<float>(NumColours);
			auto y = buttonsColourBounds.getY();
			for (auto& button : buttonsColour)
			{
				const auto bounds = BoundsF(x, y, w, hButton);
				button.setBounds(bounds.toNearestInt());
				button.label.setMaxHeight(utils.thicc);
				y += hButton;
			}
		}

		layout.place(*selector, 1, 0, 1, 1);

		const auto buttonsBottomBounds = layout(1, 1, 1, 1);
		{
			const auto w = buttonsBottomBounds.getWidth();
			const auto wButton = w / 2.f;
			const auto h = buttonsBottomBounds.getHeight();
			const auto y = buttonsBottomBounds.getY();
			auto x = buttonsBottomBounds.getX();
			buttonRevert.setBounds(BoundsF(x, y, wButton, h).toNearestInt());
			x += wButton;
			buttonDefault.setBounds(BoundsF(x, y, wButton, h).toNearestInt());
		}
	}

	void ColoursEditor::paint(Graphics& g)
	{
		g.fillAll(Colour(0xff000000));
	}

	ButtonColours::ButtonColours(ColoursEditor& menu) :
		Button(menu.utils),
		img()
	{
		value = 0.f;

		const auto paintVisor = makeButtonOnPaintVisor(2);

		makePaintButton(*this, [&, paintVisor](Graphics& g, const Button& b)
		{
			if (!img.isValid())
				return;

			paintVisor(g, b);

			const auto phaseHover = callbacks[kHoverAniCB].phase;

			const auto w = static_cast<float>(img.getWidth());
			const auto h = static_cast<float>(img.getHeight());
			const auto maniSizeRel = Pi * .25f;
			const auto wNoHover = w * maniSizeRel;
			const auto hNoHover = h * maniSizeRel;
			const auto x = wNoHover + phaseHover * (w - wNoHover);
			const auto y = hNoHover + phaseHover * (h - hNoHover);

			g.drawImage
			(
				img.rescaled(static_cast<int>(x), static_cast<int>(y), ResamplingQuality::lowResamplingQuality),
				getLocalBounds().toFloat(),
				RectanglePlacement::doNotResize,
				false
			);
		}
		, "Click here to customize the visual style of the plugin.");
		notify(evt::Type::TooltipUpdated, &tooltip);
		onClick = [&](const Mouse&)
		{
			const auto e = value < .5f;
			if (e)
				utils.eventSystem.notify(evt::Type::ClickedEmpty);
			menu.setVisible(e);
			value = std::round(1.f - value);
		};
	}

	void ButtonColours::resized()
	{
		img = Image(Image::PixelFormat::ARGB, getWidth(), getHeight(), true);
		fixStupidJUCEImageThingie(img);

		const auto bounds = maxQuadIn(getLocalBounds().toFloat());
		const auto x = bounds.getX();
		const auto y = bounds.getY();
		const auto w = bounds.getWidth();
		const auto rad = w * .5f;
		const auto radHalf = rad * .5f;
		const PointF centre(x + rad, y + rad);
		for (auto i = 0; i < 3; ++i)
		{
			const auto iR = i * .33333f;
			const auto col = Colour::fromHSV(iR, 1.f, 1.f, 1.f);
			const auto angle = iR * Tau;
			const auto lineToCircle = LineF::fromStartAndAngle(centre, radHalf * Pi * .25f, angle);
			const auto circleCentre = lineToCircle.getEnd();
			for (auto j = 0; j < img.getHeight(); ++j)
			{
				const auto jF = static_cast<float>(j);
				for (auto k = 0; k < img.getWidth(); ++k)
				{
					const auto kF = static_cast<float>(k);
					const PointF pt(kF, jF);
					if (LineF(circleCentre, pt).getLength() < radHalf)
					{
						const auto pxl = img.getPixelAt(k, j);
						if (pxl.isTransparent())
							img.setPixelAt(k, j, col);
						else
							img.setPixelAt(k, j, pxl.interpolatedWith(col, .5f));
					}

				}
			}
		}
	}

	void ColoursEditor::updateCursor(CID cID)
	{
		if (cID == CID::Interact)
			notify(evt::Type::InteractColourChanged);
		setMouseCursor(makeCursor());
	}
}