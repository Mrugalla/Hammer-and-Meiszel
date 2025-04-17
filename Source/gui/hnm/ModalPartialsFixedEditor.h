#pragma once
#include "KnobHnM.h"

namespace gui
{
	struct ModalPartialsFixedEditor :
		public Comp
	{
		ModalPartialsFixedEditor(Utils&);

		void paint(Graphics&) override;

		void resized() override;
	private:
		Label title, decayLabel, gainLabel;
		Knob vowelA, vowelB;
		KnobHnM vowelQ, vowelPos;
		Knob decay, gain;
		ModDial modDialVowelA, modDialVowelB, modDialDecay, modDialGain;
		LabelGroup group;
	};
}
