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

	////// LOOK AND FEEL:

	std::function<float(float)> makeValAlphaFunc(Button::Type type, float valAlphaMax)
	{
		switch (type)
		{
		case Button::Type::kTrigger:
			return [valAlphaMax](float)
			{
				return 0.f;
			};
		case Button::Type::kToggle:
			return [valAlphaMax](float val)
			{
				return val >= .5f ? valAlphaMax : 0.f;
			};
		default:
			return [valAlphaMax](float)
			{
				return valAlphaMax;
			};
		}
	}

	Button::OnPaint makeButtonOnPaint(Button::Type type) noexcept
	{
		const auto valAlphaMax = .4f;
		
		return [valAlphaFunc = makeValAlphaFunc(type, valAlphaMax), alphaGain = valAlphaMax * .5f]
		(Graphics& g, const Button& b)
		{
			const auto& utils = b.utils;
			const auto thicc = utils.thicc;
			const auto bounds = b.getLocalBounds().toFloat().reduced(thicc);

			auto hoverAniPhase = b.callbacks[Button::kHoverAniCB].phase;
			auto clickAniPhase = b.callbacks[Button::kClickAniCB].phase;
			hoverAniPhase = hoverAniPhase * hoverAniPhase;
			clickAniPhase = clickAniPhase * clickAniPhase;

			const auto valueAlpha = valAlphaFunc(b.value);
			const auto alpha = hoverAniPhase * alphaGain + clickAniPhase * alphaGain * .5f + valueAlpha;
			const auto aniCol = getColour(CID::Interact).withAlpha(alpha);
			g.setColour(aniCol);
			
			const auto stripesWeightState = 6.f + b.value * (2.f - 6.f);
			const auto numStripes = 1.f + hoverAniPhase * stripesWeightState + clickAniPhase * stripesWeightState;
			const auto numStripesInv = 1.f / numStripes;
			const auto stripeHeight = bounds.getHeight();
			const auto stripeY = bounds.getY();
			const auto stripeWidth = bounds.getWidth() * numStripesInv;
			const auto stripeMargin = thicc * hoverAniPhase;
			auto x = 0.f;
			for (auto i = 0.f; i < numStripes; ++i)
			{
				BoundsF stripe(x, stripeY, stripeWidth, stripeHeight);
				g.fillRoundedRectangle(stripe.reduced(stripeMargin), thicc + thicc * 3.f);
				x += stripeWidth;
			}
		};
	}

	void makeTextButton(Button& btn, const String& txt, const String& tooltip, CID cID)
	{
		makeTextLabel(btn.label, txt, font::dosisBold(), Just::centred, cID);
		btn.tooltip = tooltip;
		btn.onPaint = makeButtonOnPaint(btn.type);
	}

	void makePaintButton(Button& btn, const Label::OnPaint& onPaint, const String& tooltip)
	{
		makePaintLabel(btn.label, onPaint);
		btn.tooltip = tooltip;
		btn.onPaint = makeButtonOnPaint(btn.type);
	}

	////// PARAMETER ATTACHMENT:

	std::function<CID(float)> makeCIDFunc(Button::Type type)
	{
		switch (type)
		{
		case Button::Type::kTrigger:
			return [](float)
			{
				return CID::Interact;
			};
		case Button::Type::kToggle:
			return [](float val)
				{
				return val >= .5f ? CID::Bg : CID::Interact;
			};
		default:
			return [](float)
				{
				return CID::Bg;
			};
		}
	}

	std::function<String()> makeValToNameFunc(Button& button, PID pID)
	{
		switch (button.type)
		{
		case Button::Type::kTrigger:
			return [&btn = button]() { return btn.getName(); };
		default:
			return [&btn = button, pID]()
			{
				const auto& utils = btn.utils;
				const auto& prm = utils.getParam(pID);

				return btn.getName() + ": " + prm.getCurrentValueAsText();
			};
		}
	}

	void makeParameter(Button& button, PID pID, Button::Type type, const String& name)
	{
		auto& utils = button.utils;
		auto& param = utils.getParam(pID);
		button.value = param.getValue();

		button.type = type;
		button.setName(name.isEmpty() ? param::toString(pID) : name);
		const auto valToNameFunc = makeValToNameFunc(button, pID);

		makeTextButton(button, valToNameFunc(), param::toTooltip(pID), CID::Interact);
		
		const auto cIDFunc = makeCIDFunc(type);
		button.label.cID = cIDFunc(button.value);

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

		button.add(Callback([&btn = button, pID, cIDFunc, valToNameFunc]()
		{
			const auto& utils = btn.utils;
			const auto& param = utils.getParam(pID);
			const auto val = param.getValue();

			if (btn.value == val)
				return;

			btn.value = val;
			btn.label.cID = cIDFunc(val);
			btn.label.setText(valToNameFunc());
			btn.repaint();

		}, Button::kUpdateParameterCB, cbFPS::k15, true));
	}
}