#include "ModalParamsEditor.h"

namespace gui
{
	ModalParamsEditor::OctSemiSlider::OctSemiSlider(Utils& u, PID pID, String&& name) :
		Comp(u),
		label(u),
		knob(u),
		modDial(u)
	{
		layout.init
		(
			{ 5, 1 },
			{ 2, 1 }
		);

		addAndMakeVisible(label);
		addAndMakeVisible(knob);
		addAndMakeVisible(modDial);
		makeSlider(pID, knob);
		modDial.attach(pID);
		makeTextLabel(label, name, font::dosisBold(), Just::topLeft, CID::Txt);
		modDial.verticalDrag = false;
	}

	void ModalParamsEditor::OctSemiSlider::resized()
	{
		layout.resized(getLocalBounds());
		layout.place(label, 0.1f, .5f, .5f, 1.5f);
		layout.place(knob, 0, 0, 1, 1);
		locateAtSlider(modDial, knob);
	}

	//

	ModalParamsEditor::ModalParamsEditor(Utils& u) :
		Comp(u),
		octModal(u, PID::ModalOct, "M Oct"),
		semiModal(u, PID::ModalSemi, "M Semi"),
		octComb(u, PID::CombOct, "C Oct"),
		semiComb(u, PID::CombSemi, "C Semi"),
		unisonComb(u, PID::CombUnison, "C Unison"),
		keytrack(u, PID::ModalKeytrack, PID::ModalKeytrackEnv, PID::ModalKeytrackBreite, "Keytrack"),
		blend(u, PID::ModalBlend, PID::ModalBlendEnv, PID::ModalBlendBreite, "Blend"),
		spreizung(u, PID::ModalSpreizung, PID::ModalSpreizungEnv, PID::ModalSpreizungBreite, "Spreizung"),
		harmonie(u, PID::ModalHarmonie, PID::ModalHarmonieEnv, PID::ModalHarmonieBreite, "Harmonie"),
		kraft(u, PID::ModalKraft, PID::ModalKraftEnv, PID::ModalKraftBreite, "Kraft"),
		reso(u, PID::ModalResonanz, PID::ModalResonanzEnv, PID::ModalResonanzBreite, "Reso"),
		resoDamp(u, PID::ModalResoDamp, PID::ModalResoDampEnv, PID::ModalResoDampBreite, "Reso Damp"),
		feedback(u, PID::CombFeedback, PID::CombFeedbackEnv, PID::CombFeedbackWidth, "Feedback"),
		octSemiGroup(),
		knobLabelsGroup()
	{
		layout.init
		(
			{ 1, 1, 1, 1, 1, 1, 1, 1 },
			{ 1, 8 }
		);

		addAndMakeVisible(octModal);
		addAndMakeVisible(semiModal);
		addAndMakeVisible(octComb);
		addAndMakeVisible(semiComb);
		addAndMakeVisible(unisonComb);
		octSemiGroup.add(octModal.label);
		octSemiGroup.add(semiModal.label);
		octSemiGroup.add(octComb.label);
		octSemiGroup.add(semiComb.label);
		octSemiGroup.add(unisonComb.label);

		addAndMakeVisible(keytrack);
		addAndMakeVisible(blend);
		addAndMakeVisible(spreizung);
		addAndMakeVisible(harmonie);
		addAndMakeVisible(kraft);
		addAndMakeVisible(reso);
		addAndMakeVisible(resoDamp);
		addAndMakeVisible(feedback);

		knobLabelsGroup.add(keytrack.labelMain);
		knobLabelsGroup.add(blend.labelMain);
		knobLabelsGroup.add(spreizung.labelMain);
		knobLabelsGroup.add(harmonie.labelMain);
		knobLabelsGroup.add(kraft.labelMain);
		knobLabelsGroup.add(reso.labelMain);
		knobLabelsGroup.add(resoDamp.labelMain);
		knobLabelsGroup.add(feedback.labelMain);
	}

	void ModalParamsEditor::resized()
	{
		layout.resized(getLocalBounds());
		{
			const auto sliderArea = layout(0, 0.f, 8, 1.1f);
			const auto y = sliderArea.getY();
			const auto w = sliderArea.getWidth() / 5.f;
			const auto h = sliderArea.getHeight();
			auto x = 0.f;
			octModal.setBounds(BoundsF(x, y, w, h).toNearestInt());
			x += w;
			semiModal.setBounds(BoundsF(x, y, w, h).toNearestInt());
			x += w;
			octComb.setBounds(BoundsF(x, y, w, h).toNearestInt());
			x += w;
			semiComb.setBounds(BoundsF(x, y, w, h).toNearestInt());
			x += w;
			unisonComb.setBounds(BoundsF(x, y, w, h).toNearestInt());
		}
		octSemiGroup.setMaxHeight();

		layout.place(keytrack, 0, 1, 1, 1);
		layout.place(blend, 1, 1, 1, 1);
		layout.place(spreizung, 2, 1, 1, 1);
		layout.place(harmonie, 3, 1, 1, 1);
		layout.place(kraft, 4, 1, 1, 1);
		layout.place(reso, 5, 1, 1, 1);
		layout.place(resoDamp, 6, 1, 1, 1);
		layout.place(feedback, 7, 1, 1, 1);

		knobLabelsGroup.setMaxHeight(utils.thicc);
	}
}