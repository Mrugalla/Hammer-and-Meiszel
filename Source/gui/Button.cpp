#include "Button.h"

namespace gui
{
	Button::Button(Utils& u) :
		Comp(u),
		label(u),
		onPaint([](Graphics&, const Button&) {}),
		onClick([](const Mouse&) {}),
		onWheel([](const Mouse&, const MouseWheel&) {}),
		value(0.f),
		type(Type::kTrigger)
	{
		addAndMakeVisible(label);

		const auto fps = cbFPS::k30;
		const auto speed = msToInc(AniLengthMs, fps);

		add(Callback([&, speed]()
		{
			auto& phase = callbacks[kHoverAniCB].phase;
			const auto hovering = isMouseOverOrDragging();
			if (hovering)
			{
				phase += speed;
				if (phase >= 1.f)
					callbacks[kHoverAniCB].stop(1.f);
			}
			else
			{
				phase -= speed;
				if (phase <= 0.f)
					callbacks[kHoverAniCB].stop(0.f);
			}
			repaint();
		}, kHoverAniCB, fps, false));

		add(Callback([&, speed]()
		{
			auto& phase = callbacks[kClickAniCB].phase;
			phase -= speed;
			if (phase <= 0.f)
			{
				phase = 0.f;
				callbacks[kClickAniCB].active = false;
			}
			repaint();
		}, kClickAniCB, fps, false));

		add(Callback([&, speed]()
		{
			auto& toggleState = callbacks[kToggleStateCB].phase;
			if (type != Type::kToggle)
			{
				if (toggleState != 0.f)
				{
					toggleState = 0.f;
					callbacks[kToggleStateCB].stop(0.f);
					repaint();
				}
				return;
			}
			const auto vRound = std::round(value);
			const auto dist = vRound - toggleState;
			const auto dif = dist * dist;
			const auto eps = .001f;
			if (dif > eps)
			{
				toggleState += speed * (vRound - toggleState);
				repaint();
			}
			else if (toggleState != value)
			{
				toggleState = vRound;
				repaint();
			}
		}, kToggleStateCB, fps, true));

		callbacks[kHoverAniCB].phase = 0.f;
	}

	void Button::paint(Graphics& g)
	{
		onPaint(g, *this);
	}

	const String& Button::getText() const noexcept
	{
		return label.text;
	}

	String& Button::getText() noexcept
	{
		return label.text;
	}

	void Button::resized()
	{
		label.setBounds(getLocalBounds().toFloat().reduced(utils.thicc).toNearestInt());
		if(label.type == Label::Type::Text)
			label.setMaxHeight();
	}

	void Button::mouseEnter(const Mouse& mouse)
	{
		Comp::mouseEnter(mouse);
		callbacks[kHoverAniCB].start(callbacks[kHoverAniCB].phase);
		repaint();
	}

	void Button::mouseExit(const Mouse& mouse)
	{
		Comp::mouseExit(mouse);
		callbacks[kHoverAniCB].start(callbacks[kHoverAniCB].phase);
	}

	void Button::mouseDown(const Mouse& mouse)
	{
		Comp::mouseDown(mouse);
		repaint();
	}

	void Button::mouseUp(const Mouse& mouse)
	{
		utils.giveDAWKeyboardFocus();

		if (mouse.mouseWasDraggedSinceMouseDown())
			return;

		callbacks[kClickAniCB].start(1.f);
		onClick(mouse);
		repaint();
	}

	void Button::mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel)
	{
		onWheel(mouse, wheel);
	}

	////// ONPAINTS:
	
	Button::OnPaint makeButtonOnPaint(bool drawToggle, Colour bgCol)
	{
		return [drawToggle, bgCol] (Graphics& g, const Button& b)
		{
			const auto& utils = b.utils;
			const auto thicc = utils.thicc;
			const auto thicc2 = thicc * 2.f;
			
			const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
			const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;
			const auto togglePhase = b.callbacks[Button::kToggleStateCB].phase;

			const auto lineThiccness = thicc + clickPhase * (thicc2 - thicc);
			const auto bounds = b.getLocalBounds().toFloat().reduced(lineThiccness);

			auto bgColour = bgCol;
			if(drawToggle)
				bgColour = bgColour.interpolatedWith(getColour(CID::Interact), togglePhase * .3f);
			bgColour = bgColour.overlaidWith(getColour(CID::Interact).withAlpha(hoverPhase * .3f));

			g.setColour(bgColour);
			g.fillRoundedRectangle(bounds, lineThiccness);
		};
	}

	Button::OnPaint makeButtonOnPaintPower()
	{
		return [op = makeButtonOnPaint(false, getColour(CID::Bg))] (Graphics& g, const Button& b)
		{
			op(g, b);

			const auto& utils = b.utils;
			const auto thicc = utils.thicc;
			const auto thicc2 = thicc * 2.f;
			const auto thicc4 = thicc * 4.f;

			const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
			const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;
			const auto togglePhase = b.callbacks[Button::kToggleStateCB].phase;

			const auto lineThiccness = thicc + togglePhase * (thicc2 - thicc);
			const auto margin = 1.25f * thicc4 - lineThiccness - hoverPhase * thicc;
			const auto bounds = maxQuadIn(b.getLocalBounds().toFloat()).reduced(margin);

			const auto x = bounds.getX();
			const auto y = bounds.getY();
			const auto rad = bounds.getWidth() * .5f;

			const PointF centre
			(
				x + rad,
				y + rad
			);

			const auto fromRads = Pi * .2f;
			const auto toRads = Tau - fromRads;

			Path path;
			path.addCentredArc
			(
				centre.x,
				centre.y,
				rad,
				rad,
				0.f,
				fromRads,
				toRads,
				true
			);

			const Stroke stroke(lineThiccness, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
			auto linesColour = getColour(CID::Interact).overlaidWith(getColour(CID::Txt).withAlpha(clickPhase));
			g.setColour(linesColour);
			g.strokePath(path, stroke);
			const LineF line(centre, centre.withY(y));
			g.drawLine(line, lineThiccness);
		};
	}

	Button::OnPaint makeButtonOnPaintPolarity()
	{
		return [op = makeButtonOnPaint(false, getColour(CID::Bg))] (Graphics& g, const Button& b)
		{
			op(g, b);

			const auto& utils = b.utils;
			const auto thicc = utils.thicc;
			const auto thicc2 = thicc * 2.f;

			const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
			const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;
			const auto togglePhase = b.callbacks[Button::kToggleStateCB].phase;

			const auto lineThiccness = thicc + togglePhase * (thicc2 - thicc);
			const auto margin = 2.5f * thicc2 - lineThiccness - hoverPhase * thicc;
			const auto bounds = maxQuadIn(b.getLocalBounds().toFloat()).reduced(margin);

			auto linesColour = getColour(CID::Interact).overlaidWith(getColour(CID::Txt).withAlpha(clickPhase));
			g.setColour(linesColour);
			g.drawEllipse(bounds, lineThiccness);
			const LineF line(bounds.getBottomLeft(), bounds.getTopRight());
			g.drawLine(line, lineThiccness);
		};
	}

	Button::OnPaint makeButtonOnPaintSwap()
	{
		return [op = makeButtonOnPaint(false, getColour(CID::Bg))](Graphics& g, const Button& b)
			{
				op(g, b);

				const auto& utils = b.utils;
				const auto thicc = utils.thicc;
				const auto thicc2 = thicc * 2.f;
				const auto thicc4 = thicc * 4.f;

				const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
				const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;
				const auto togglePhase = b.callbacks[Button::kToggleStateCB].phase;

				const auto lineThiccness = thicc + togglePhase * (thicc2 - thicc);
				const auto margin = thicc4 - lineThiccness - hoverPhase * thicc;
				const auto bounds = maxQuadIn(b.getLocalBounds().toFloat()).reduced(margin);

				auto linesColour = getColour(CID::Interact).overlaidWith(getColour(CID::Txt).withAlpha(clickPhase));
				g.setColour(linesColour);
				
				float xVal[2] =
				{
					bounds.getRight(),
					bounds.getX()
				};

				for (auto i = 0; i < 2; ++i)
				{
					const auto iF = static_cast<float>(i);
					const auto r = (iF + 1.f) * .33333f;

					const auto y = bounds.getY() + bounds.getHeight() * r;
					const auto x0 = xVal[i];
					const auto x1 = xVal[(i + 1) % 2];

					const LineF line(x0, y, x1, y);
					g.drawLine(line, thicc);

					const auto pt = line.getEnd();

					const auto ang0 = iF * PiHalf;

					for (auto j = 0; j < 2; ++j)
					{
						const auto jF = static_cast<float>(j);
						const auto jF2 = jF * 2.f;

						const auto angle = (1.f + jF2) * PiQuart + ang0 * (-1.f + jF2);

						const auto tick = LineF::fromStartAndAngle(pt, thicc, angle)
							.withLengthenedStart(thicc * .5f)
							.withLengthenedEnd(thicc);
						g.drawLine(tick, thicc);
					}
				}
			};
	}

	Button::OnPaint makeButtonOnPaintVisor(int numDiagonalLines)
	{
		return [numDiagonalLines](Graphics& g, const Button& b)
		{
			const auto thicc = b.utils.thicc;
			const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
			const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;

			const auto bounds = b.getLocalBounds().toFloat();
			const auto w = bounds.getWidth();
			const auto h = bounds.getHeight();
			const auto minDimen = std::min(w, h);

			const auto len = hoverPhase * minDimen * (.2f + clickPhase * .1f);

			setCol(g, CID::Interact);
			g.drawLine(LineF(0.f, 0.f, len, 0.f), thicc);
			g.drawLine(LineF(0.f, 0.f, 0.f, len), thicc);
			g.drawLine(LineF(0.f, h, len, h), thicc);
			g.drawLine(LineF(0.f, h, 0.f, h - len), thicc);
			g.drawLine(LineF(w, 0.f, w - len, 0.f), thicc);
			g.drawLine(LineF(w, 0.f, w, len), thicc);
			g.drawLine(LineF(w, h, w - len, h), thicc);
			g.drawLine(LineF(w, h, w, h - len), thicc);

			const auto numDiagonalLinesInv = 1.f / static_cast<float>(numDiagonalLines);
			for (auto i = 1; i <= numDiagonalLines; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto iR = iF * numDiagonalLinesInv;
				const auto iLen = len * iR;
				g.drawLine(LineF(0.f, iLen, iLen, 0.f), thicc);
				g.drawLine(LineF(w, h - iLen, w - iLen, h), thicc);
				g.drawLine(LineF(0.f, h - iLen, iLen, h), thicc);
				g.drawLine(LineF(w, iLen, w - iLen, 0.f), thicc);
			}
		};
	}

	////// LOOK AND FEEL:

	void makeTextButton(Button& btn, const String& txt, const String& tooltip, CID cID, Colour bgCol)
	{
		makeTextLabel(btn.label, txt, font::dosisBold(), Just::centred, cID);
		btn.tooltip = tooltip;
		btn.onPaint = makeButtonOnPaint(true, bgCol);
	}

	void makePaintButton(Button& btn, const Button::OnPaint& onPaint, const String& tooltip)
	{
		btn.onPaint = onPaint;
		btn.setTooltip(tooltip);
	}

	////// PARAMETER ATTACHMENT:

	std::function<String()> makeValToNameFunc(Button& button, PID pID, const String& text)
	{
		switch (button.type)
		{
		case Button::Type::kTrigger:
			return [&btn = button, text]()
				{
					if(text.isEmpty())
						return btn.getName();
					return text;
				};
		default:
			return [&btn = button, pID, text]()
			{
				const auto& utils = btn.utils;
				const auto& prm = utils.getParam(pID);
				if(text.isEmpty())
					return prm.getCurrentValueAsText();
				for(auto i = 0; i < text.length(); ++i)
					if (text[i] == ';')
					{
						const auto val = prm.getValue() > .5f;
						return val ? text.substring(i + 1) : text.substring(0, i);
					}
				return text;
			};
		}
	}

	void makeParameter(Button& button, PID pID)
	{
		auto& utils = button.utils;
		auto& param = utils.getParam(pID);
		button.value = param.getValue();

		button.tooltip = param::toTooltip(pID);

		const auto valChangeFunc = [&prm = param](int valDenorm)
		{
			const auto start = static_cast<int>(prm.range.start);
			const auto end = static_cast<int>(prm.range.end);
			if (valDenorm > end)
				valDenorm = start;
			else if (valDenorm < start)
				valDenorm = end;
			const auto valNorm = prm.range.convertTo0to1(static_cast<float>(valDenorm));
			prm.setValueWithGesture(valNorm);
		};

		button.onClick = [&btn = button, pID, valChangeFunc](const Mouse& mouse)
		{
			auto& utils = btn.utils;
			auto& param = utils.getParam(pID);
			const auto& range = param.range;
			const auto direc = mouse.mods.isRightButtonDown() ? -1 : 1;
			const auto interval = static_cast<int>(range.interval) * direc;
			auto valDenorm = static_cast<int>(param.getValueDenorm()) + interval;
			valChangeFunc(valDenorm);
		};

		button.onWheel = [&btn = button, pID, valChangeFunc](const Mouse&, const MouseWheel& wheel)
		{
			auto& utils = btn.utils;
			auto& param = utils.getParam(pID);
			const auto& range = param.range;
			const auto interval = static_cast<int>(range.interval);
			const auto direc = (wheel.deltaY > 0.f ? 1 : -1) * (wheel.isReversed ? -1 : 1);
			auto valDenorm = static_cast<int>(param.getValueDenorm()) + interval * direc;
			valChangeFunc(valDenorm);
		};
	}

	void makeParameter(Button& button, PID pID, Button::Type type, const String& text)
	{
		makeParameter(button, pID);

		button.type = type;
		button.setName(param::toString(pID));
		const auto valToNameFunc = makeValToNameFunc(button, pID, text);

		makeTextButton(button, valToNameFunc(), param::toTooltip(pID), CID::Interact);

		button.label.cID = CID::Interact;
		button.add(Callback([&btn = button, pID, valToNameFunc]()
		{
			const auto& utils = btn.utils;
			const auto& param = utils.getParam(pID);
			const auto val = param.getValue();

			if (btn.value == val)
				return;

			btn.value = val;
			btn.label.setText(valToNameFunc());
			btn.repaint();
		}, Button::kUpdateParameterCB, cbFPS::k15, true));
	}

	void makeParameter(Button& button, PID pID, Button::Type type, Button::OnPaint onPaint)
	{
		makeParameter(button, pID);

		button.type = type;
		button.onPaint = onPaint;
		button.setTooltip(param::toTooltip(pID));

		button.add(Callback([&btn = button, pID]()
		{
			const auto& utils = btn.utils;
			const auto& param = utils.getParam(pID);
			const auto val = param.getValue();

			if (btn.value == val)
				return;

			btn.value = val;
		}, Button::kUpdateParameterCB, cbFPS::k15, true));
	}
}