#pragma once
#include "../Knob.h"

namespace gui
{
	struct KnobHnM :
		public Comp
	{
		static constexpr auto MainKnobHeightRel = .5f;
		static constexpr auto MainLabelHeightRel = .2f;
		static constexpr auto KnobLabelOverlap = .4f;

		KnobHnM(Utils& u, PID pIDMain, PID pIDEnv, PID pIDWidth,
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
				{ 5, 2, 3, 1 }
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

			modDialMain.attach(pIDMain);
			modDialEnv.attach(pIDEnv);
			modDialWidth.attach(pIDWidth);

			makeTextLabel(labelMain, _name, font::dosisMedium(), Just::centred, CID::Txt);
			makeTextLabel(labelEnv, "Env", font::dosisMedium(), Just::centred, CID::Txt);
			makeTextLabel(labelWidth, "Width", font::dosisMedium(), Just::centred, CID::Txt);

			labelGroup.add(labelEnv);
			labelGroup.add(labelWidth);
		}

		void resized() override
		{
			layout.resized(getLocalBounds().toFloat());
			layout.place(main, 0, 0.f, 2, 1.2f);
			layout.place(labelMain, 0, 1, 2, 1);
			layout.place(env, 0, 1.8f, 1, 1.4f);
			layout.place(width, 1, 1.8f, 1, 1.4f);
			layout.place(labelEnv, 0, 3, 1, 1);
			layout.place(labelWidth, 1, 3, 1, 1);

			locateAtKnob(modDialMain, main);
			locateAtKnob(modDialEnv, env);
			locateAtKnob(modDialWidth, width);
			labelGroup.setMaxHeight();
		}

		Label labelMain, labelEnv, labelWidth;
		Knob main, env, width;
		ModDial modDialMain, modDialEnv, modDialWidth;
		LabelGroup labelGroup;
	};
}