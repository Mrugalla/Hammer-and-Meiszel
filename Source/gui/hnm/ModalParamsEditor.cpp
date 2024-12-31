#include "ModalParamsEditor.h"

namespace gui
{
	ModalMainParams::ModalMainParams(Utils& u, PID smartKeytrack, PID blend,
		PID spread, PID harmonie, PID kraft, PID reso, PID resoDamp) :
		Comp(u),
		knobs{ Knob(u), Knob(u), Knob(u), Knob(u), Knob(u), Knob(u), Knob(u) },
		modDials{ ModDial(u), ModDial(u), ModDial(u), ModDial(u), ModDial(u), ModDial(u), ModDial(u) }
	{
		for (auto& knob : knobs)
			addAndMakeVisible(knob);
		for (auto& modDial : modDials)
			addAndMakeVisible(modDial);

		makeKnob(smartKeytrack, knobs[kParam::kSmartKeytrack]);
		makeKnob(blend, knobs[kParam::kBlend]);
		makeKnob(spread, knobs[kParam::kSpreizung]);
		makeKnob(harmonie, knobs[kParam::kHarmonie]);
		makeKnob(kraft, knobs[kParam::kKraft]);
		makeKnob(reso, knobs[kParam::kReso]);
		makeKnob(resoDamp, knobs[kParam::kResoDamp]);

		modDials[kParam::kSmartKeytrack].attach(smartKeytrack);
		modDials[kParam::kBlend].attach(blend);
		modDials[kParam::kSpreizung].attach(spread);
		modDials[kParam::kHarmonie].attach(harmonie);
		modDials[kParam::kKraft].attach(kraft);
		modDials[kParam::kReso].attach(reso);
		modDials[kParam::kResoDamp].attach(resoDamp);
	}

	void ModalMainParams::resized()
	{
		const auto w = static_cast<float>(getWidth());
		const auto h = static_cast<float>(getHeight());
		const auto y = 0.f;
		const auto wKnob = w / static_cast<float>(knobs.size());
		auto x = 0.f;
		for (auto& knob : knobs)
		{
			knob.setBounds(BoundsF(x, y, wKnob, h).toNearestInt());
			x += wKnob;
		}

		for (auto i = 0; i < kParam::kNumParams; ++i)
			locateAtKnob(modDials[i], knobs[i]);
	}

	//

	ModalParamsEditor::OctSemiSlider::OctSemiSlider(Utils& u, PID pID, String&& name) :
		Comp(u),
		label(u),
		knob(u),
		modDial(u)
	{
		layout.init
		(
			{ 1, 3, 1 },
			{ 1 }
		);

		addAndMakeVisible(label);
		addAndMakeVisible(knob);
		addAndMakeVisible(modDial);
		makeSlider(pID, knob);
		modDial.attach(pID);
		makeTextLabel(label, name, font::dosisBold(), Just::centredRight, CID::Txt);
		modDial.verticalDrag = false;
	}

	void ModalParamsEditor::OctSemiSlider::resized()
	{
		layout.resized(getLocalBounds());
		layout.place(label, 0, 0, 1, 1);
		layout.place(knob, 1, 0, 1, 1);
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
		smartKeytrack(u, PID::ModalSmartKeytrack, PID::ModalSmartKeytrackEnv, PID::ModalSmartKeytrackBreite, "Keytrack++"),
		blend(u, PID::ModalBlend, PID::ModalBlendEnv, PID::ModalBlendBreite, "Blend"),
		spreizung(u, PID::ModalSpreizung, PID::ModalSpreizungEnv, PID::ModalSpreizungBreite, "Spreizung"),
		harmonie(u, PID::ModalHarmonie, PID::ModalHarmonieEnv, PID::ModalHarmonieBreite, "Harmonie"),
		kraft(u, PID::ModalKraft, PID::ModalKraftEnv, PID::ModalKraftBreite, "Kraft"),
		reso(u, PID::ModalResonanz, PID::ModalResonanzEnv, PID::ModalResonanzBreite, "Reso"),
		resoDamp(u, PID::ModalResoDamp, PID::ModalResoDampEnv, PID::ModalResoDampBreite, "Reso Damp"),
		feedback(u, PID::CombFeedback, PID::CombFeedbackEnv, PID::CombFeedbackWidth, "Feedback"),
		octSemiGroup()
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

		addAndMakeVisible(smartKeytrack);
		addAndMakeVisible(blend);
		addAndMakeVisible(spreizung);
		addAndMakeVisible(harmonie);
		addAndMakeVisible(kraft);
		addAndMakeVisible(reso);
		addAndMakeVisible(resoDamp);
		addAndMakeVisible(feedback);
	}

	void ModalParamsEditor::resized()
	{
		layout.resized(getLocalBounds());
		{
			const auto sliderArea = layout.top();
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

		layout.place(smartKeytrack, 0, 1, 1, 1);
		layout.place(blend, 1, 1, 1, 1);
		layout.place(spreizung, 2, 1, 1, 1);
		layout.place(harmonie, 3, 1, 1, 1);
		layout.place(kraft, 4, 1, 1, 1);
		layout.place(reso, 5, 1, 1, 1);
		layout.place(resoDamp, 6, 1, 1, 1);
		layout.place(feedback, 7, 1, 1, 1);
	}
}