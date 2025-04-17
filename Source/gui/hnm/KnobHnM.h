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

		//u, main, env, width, name
		KnobHnM(Utils&, PID, PID, PID, String&&);

		void resized() override;

		Label labelMain, labelEnv, labelWidth;
		Knob main, env, width;
		ModDial modDialMain, modDialEnv, modDialWidth;
		LabelGroup labelGroup;
	};
}