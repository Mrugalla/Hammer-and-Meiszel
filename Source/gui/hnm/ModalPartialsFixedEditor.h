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
			decayLabel(u), gainLabel(u),
			vowelA(u), vowelB(u),
			vowelQ(u, PID::FormantQ, PID::FormantQEnv, PID::FormantQWidth, "Q"),
			vowelPos(u, PID::FormantPos, PID::FormantPosEnv, PID::FormantPosWidth, "Pos"),
			decay(u), gain(u),
			modDialVowelA(u), modDialVowelB(u), modDialDecay(u), modDialGain(u),
			group()
		{
			layout.init
			(
				{ 1, 1, 1, 1 },
				{ 1, 1, 5 }
			);

			addAndMakeVisible(title);
			addAndMakeVisible(decayLabel);
			addAndMakeVisible(gainLabel);
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

			makeTextKnob(PID::FormantA, vowelA);
			makeTextKnob(PID::FormantB, vowelB);
			makeKnob(PID::FormantDecay, decay);
			makeKnob(PID::FormantGain, gain);

			modDialVowelA.attach(PID::FormantA);
			modDialVowelB.attach(PID::FormantB);
			modDialDecay.attach(PID::FormantDecay);
			modDialGain.attach(PID::FormantGain);

			const auto labelFont = font::dosisMedium();
			makeTextLabel(title, " << Formant Filter >> ", labelFont, Just::centred, CID::Txt, "The formant filter runs in parallel with the modal filter.");
			title.autoMaxHeight = true;
			makeTextLabel(decayLabel, "Decay", labelFont, Just::centred, CID::Txt);
			makeTextLabel(gainLabel, "Gain", labelFont, Just::centred, CID::Txt);

			group.add(vowelPos.labelMain);
			group.add(vowelQ.labelMain);
			group.add(decayLabel);
			group.add(gainLabel);
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
			layout.place(decay, 2, 2.f, 1, .5f);
			layout.place(gain, 3, 2.f, 1, .5f);
			layout.place(decayLabel, 2, 2.5f, 1, .5f);
			layout.place(gainLabel, 3, 2.5f, 1, .5f);

			locateAtKnob(modDialVowelA, vowelA);
			locateAtKnob(modDialVowelB, vowelB);
			locateAtKnob(modDialDecay, decay);
			locateAtKnob(modDialGain, gain);

			group.setMaxHeight(utils.thicc);
		}
	private:
		Label title, decayLabel, gainLabel;
		Knob vowelA, vowelB;
		KnobHnM vowelQ, vowelPos;
		Knob decay, gain;
		ModDial modDialVowelA, modDialVowelB, modDialDecay, modDialGain;
		LabelGroup group;
	};
}
