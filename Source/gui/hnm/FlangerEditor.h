#pragma once
#include "../Knob.h"

namespace gui
{
	struct ModuleParameterKnobs :
		public Comp
	{
		ModuleParameterKnobs(Utils& u, PID pIDMain, PID pIDEnv, PID pIDWidth,
			String&& _name) :
			Comp(u),
			labelMain(u), labelEnv(u), labelWidth(u),
			main(u), env(u), width(u),
			modDialMain(u), modDialEnv(u), modDialWidth(u),
			labelGroup()
		{
			layout.init
			(
				{ 1, 1 },
				{ 2, 1, 1, 1 }
			);

			addAndMakeVisible(labelMain);
			addAndMakeVisible(labelEnv);
			addAndMakeVisible(labelWidth);
			addAndMakeVisible(main);
			addAndMakeVisible(modDialMain);
			addAndMakeVisible(env);
			addAndMakeVisible(modDialEnv);
			addAndMakeVisible(width);
			addAndMakeVisible(modDialWidth);

			makeKnob(main);
			makeKnob(env);
			makeKnob(width);

			makeParameter(pIDMain, main);
			makeParameter(pIDEnv, env);
			makeParameter(pIDWidth, width);

			modDialMain.verticalDrag = false;
			modDialEnv.verticalDrag = false;
			modDialWidth.verticalDrag = false;
			modDialMain.attach(pIDMain);
			modDialEnv.attach(pIDEnv);
			modDialWidth.attach(pIDWidth);

			makeTextLabel(labelMain, _name, font::dosisMedium(), Just::centred, CID::Txt);
			makeTextLabel(labelEnv, "Env", font::dosisMedium(), Just::centred, CID::Txt);
			makeTextLabel(labelWidth, "Width", font::dosisMedium(), Just::centred, CID::Txt);

			labelGroup.add(labelMain);
			labelGroup.add(labelEnv);
			labelGroup.add(labelWidth);
		}

		void resized() override
		{
			layout.resized(getLocalBounds().toFloat());

			layout.place(main, 0, 0, 2, 1);
			layout.place(labelMain, 0, 1, 2, 1);
			layout.place(env, 0, 2, 1, 1);
			layout.place(width, 1, 2, 1, 1);
			layout.place(labelEnv, 0, 3, 1, 1);
			layout.place(labelWidth, 1, 3, 1, 1);

			locateAtKnob(modDialMain, main);
			locateAtKnob(modDialEnv, env);
			locateAtKnob(modDialWidth, width);

			labelGroup.setMaxHeight(utils.thicc);
		}

		Label labelMain, labelEnv, labelWidth;
		Knob main, env, width;
		ModDial modDialMain, modDialEnv, modDialWidth;
		LabelGroup labelGroup;
	};

	struct FlangerEditor :
		public Comp
	{
		FlangerEditor(Utils& u) :
			Comp(u),
			labelTitle(u),
			labelOct(u),
			labelSemi(u),
			oct(u),
			semi(u),

			modDialOct(u),
			modDialSemi(u),
			feedback(u, PID::CombFeedback, PID::CombFeedbackEnv, PID::CombFeedbackWidth, "Feedback"),
			apReso(u, PID::CombAPReso, PID::CombAPResoEnv, PID::CombAPResoWidth, "AP Reso"),
			apShape(u, PID::CombAPShape, PID::CombAPShapeEnv, PID::CombAPShapeWidth, "AP Shape")
		{
			layout.init
			(
				{ 1, 1, 1 },
				{ 1, 8 }
			);

			addAndMakeVisible(labelTitle);
			addAndMakeVisible(labelOct);
			addAndMakeVisible(labelSemi);
			addAndMakeVisible(oct);
			addAndMakeVisible(modDialOct);
			addAndMakeVisible(semi);
			addAndMakeVisible(modDialSemi);
			addAndMakeVisible(feedback);
			addAndMakeVisible(apReso);
			addAndMakeVisible(apShape);
			
			makeSlider(oct);
			makeSlider(semi);

			makeParameter(PID::CombOct, oct, false);
			makeParameter(PID::CombSemi, semi, false);

			modDialOct.attach(PID::CombOct);
			modDialSemi.attach(PID::CombSemi);

			makeTextLabel(labelTitle, "Comb Filter:", font::dosisBold(), Just::centred, CID::Txt);
			makeTextLabel(labelOct, "Oct", font::dosisMedium(), Just::centred, CID::Txt);
			makeTextLabel(labelSemi, "Semi", font::dosisMedium(), Just::centred, CID::Txt);
		}

		void paint(Graphics& g) override
		{
			setCol(g, CID::Bg);
			g.fillAll();
		}

		void resized() override
		{
			layout.resized(getLocalBounds().toFloat());
			layout.place(labelTitle, 0, 0, 1, 1);
			labelTitle.setMaxHeight(utils.thicc);
			layout.place(labelOct, 1.f, 0, .2f, 1);
			layout.place(oct, 1.2f, 0, .6f, 1);
			layout.place(labelSemi, 2.f, 0, .2f, 1);
			layout.place(semi, 2.2f, 0, .6f, 1);
			layout.place(feedback, 0, 1, 1, 1);
			layout.place(apReso, 1, 1, 1, 1);
			layout.place(apShape, 2, 1, 1, 1);
			locateAtSlider(modDialOct, oct);
			locateAtSlider(modDialSemi, semi);
		}

		Label labelTitle;
		Label labelOct, labelSemi;
		Knob oct, semi;
		ModDial modDialOct, modDialSemi;
		ModuleParameterKnobs feedback, apReso, apShape;
	};
}