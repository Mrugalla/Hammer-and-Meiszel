#pragma once
#include "Label.h"

namespace gui
{
	struct Button :
		public Comp
	{
		using OnPaint = std::function<void(Graphics&, const Button&)>;
		using OnClick = std::function<void(const Mouse&)>;
		using OnWheel = std::function<void(const Mouse&, const MouseWheel&)>;
		enum class Type { kTrigger, kToggle, kChoice, kNumTypes };
		enum { kHoverAniCB, kClickAniCB, kToggleStateCB, kUpdateParameterCB, kNumCallbacks };

		Button(Utils&);

		void paint(Graphics&) override;

		const String& getText() const noexcept;

		String& getText() noexcept;

		void resized() override;

		void mouseEnter(const Mouse&) override;

		void mouseExit(const Mouse&) override;

		void mouseDown(const Mouse&) override;

		void mouseUp(const Mouse&) override;

		void mouseWheelMove(const Mouse&, const MouseWheel&) override;

		Label label;
		OnPaint onPaint;
		OnClick onClick;
		OnWheel onWheel;
		float value;
		Type type;
	};

	////// ONPAINTS:
	
	// drawToggle, bgColour
	Button::OnPaint makeButtonOnPaint(bool, Colour);

	Button::OnPaint makeButtonOnPaintPower();

	Button::OnPaint makeButtonOnPaintPolarity();

	Button::OnPaint makeButtonOnPaintSwap();

	Button::OnPaint makeButtonOnPaintClip();

	// numDiagnoalLines
	Button::OnPaint makeButtonOnPaintVisor(int);

	////// LOOK AND FEEL:

	// btn, text, tooltip, cID, bgCol
	void makeTextButton(Button&, const String&, const String&, CID, Colour = getColour(CID::Bg));

	// btn, onPaint, tooltip
	void makePaintButton(Button&, const Button::OnPaint&, const String&);

	////// PARAMETER ATTACHMENT:

	// button, PID
	void makeParameter(Button&, PID);

	// button, PID, type, name
	void makeParameter(Button&, PID, Button::Type, const String& = "");

	// button, PID, type, onPaint
	void makeParameter(Button&, PID, Button::Type, Button::OnPaint);

	void makeParameter(std::vector<std::unique_ptr<Button>>&, PID);
}