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
		enum { kHoverAniCB, kClickAniCB, kUpdateParameterCB, kNumCallbacks };

		/* u */
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

	/* type */
	Button::OnPaint makeButtonOnPaint(Button::Type) noexcept;

	////// LOOK AND FEEL:

	/* btn, text, tooltip, cID */
	void makeTextButton(Button&, const String&, const String&, CID);

	/* btn, onPaint, tooltip */
	void makePaintButton(Button&, const Label::OnPaint&, const String&);

	////// PARAMETER ATTACHMENT:

	/* button, PID, type, name */
	void makeParameter(Button&, PID, Button::Type, const String& = "");
}