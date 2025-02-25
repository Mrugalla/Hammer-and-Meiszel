#pragma once
#include "KnobHnM.h"

namespace gui
{
	struct ModalPartialsFixedEditor :
		public Comp
	{
		ModalPartialsFixedEditor(Utils& u) :
			Comp(u),
			title(u),
			vowelA(u), vowelB(u),
			vowelQ(u, PID::FormantQ, PID::FormantQEnv, PID::FormantQWidth, "Q"),
			vowelPos(u, PID::FormantPos, PID::FormantPosEnv, PID::FormantPosWidth, "Pos"),
			decay(u), gain(u),
			modDialVowelA(u), modDialVowelB(u), modDialDecay(u), modDialGain(u)
		{
			layout.init
			(
				{ 3, 3, 2, 2 },
				{ 1, 2, 5 }
			);

			addAndMakeVisible(title);
			addAndMakeVisible(vowelA);
			addAndMakeVisible(vowelB);
			addAndMakeVisible(vowelPos);
			addAndMakeVisible(vowelQ);
			addAndMakeVisible(decay);
			addAndMakeVisible(gain);
			addAndMakeVisible(modDialVowelA);
			addAndMakeVisible(modDialVowelB);
			addAndMakeVisible(modDialDecay);
			addAndMakeVisible(modDialGain);

			makeKnob(PID::FormantA, vowelA);
			makeKnob(PID::FormantB, vowelB);
			makeKnob(PID::FormantDecay, decay);
			makeKnob(PID::FormantGain, gain);

			modDialVowelA.attach(PID::FormantA);
			modDialVowelB.attach(PID::FormantB);
			modDialDecay.attach(PID::FormantDecay);
			modDialGain.attach(PID::FormantGain);

			const auto labelFont = font::dosisMedium();
			makeTextLabel(title, "Formant Filter:", labelFont, Just::centred, CID::Txt, "The formant filter runs in parallel with the modal filter.");
			title.autoMaxHeight = true;
			//makeTextLabel(labelVowelA, "A", labelFont, Just::centred, CID::Interact);
			//makeTextLabel(labelVowelB, "B", labelFont, Just::centred, CID::Interact);
			//makeTextLabel(labelDecay, "Decay", labelFont, Just::centred, CID::Interact);
		}

		void paint(Graphics& g) override
		{
			setCol(g, CID::Bg);
			g.fillAll();
		}

		void resized() override
		{
			layout.resized(getLocalBounds().toFloat());
			layout.place(title, 0, 0, 4, 1);
			const auto vowelsArea = layout(0, 1, 4, 1);
			vowelA.setBounds(vowelsArea.withWidth(vowelsArea.getWidth() * .5f).toNearestInt());
			vowelB.setBounds(vowelA.getBounds().withX(vowelA.getRight()));
			layout.place(vowelQ, 0, 2, 1, 1);
			layout.place(vowelPos, 1, 2, 1, 1);
			layout.place(decay, 2, 2, 1, 1);
			layout.place(gain, 3, 2, 1, 1);

			locateAtKnob(modDialVowelA, vowelA);
			locateAtKnob(modDialVowelB, vowelB);
			locateAtKnob(modDialDecay, decay);
			locateAtKnob(modDialGain, gain);
		}
	private:
		Label title;
		Knob vowelA, vowelB;
		KnobHnM vowelQ, vowelPos;
		Knob decay, gain;
		ModDial modDialVowelA, modDialVowelB, modDialDecay, modDialGain;
	};
}
