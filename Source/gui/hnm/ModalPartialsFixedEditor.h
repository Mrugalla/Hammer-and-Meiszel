#pragma once
#include "../Knob.h"

namespace gui
{
	struct ModalPartialsFixedEditor :
		public Comp
	{
		ModalPartialsFixedEditor(Utils& u) :
			Comp(u),
			knobPos(u), knobQ(u), knobGain(u), knobVowelA(u), knobVowelB(u),
			knobPosEnv(u), knobQEnv(u), knobGainEnv(u),
			modDialPos(u), modDialQ(u), modDialGain(u), modDialVowelA(u), modDialVowelB(u),
			modDialPosEnv(u), modDialQEnv(u), modDialGainEnv(u),
			labelPos(u), labelQ(u), labelGain(u), labelVowelA(u), labelVowelB(u),
			labelPosEnv(u), labelQEnv(u), labelGainEnv(u),
			labelGroup()
		{
			addAndMakeVisible(labelPos);
			addAndMakeVisible(labelQ);
			addAndMakeVisible(labelGain);
			addAndMakeVisible(labelVowelA);
			addAndMakeVisible(labelVowelB);
			addAndMakeVisible(labelPosEnv);
			addAndMakeVisible(labelQEnv);
			addAndMakeVisible(labelGainEnv);

			addAndMakeVisible(knobPos);
			addAndMakeVisible(knobQ);
			addAndMakeVisible(knobGain);
			addAndMakeVisible(knobVowelA);
			addAndMakeVisible(knobVowelB);
			addAndMakeVisible(knobPosEnv);
			addAndMakeVisible(knobQEnv);
			addAndMakeVisible(knobGainEnv);

			addAndMakeVisible(modDialPos);
			addAndMakeVisible(modDialQ);
			addAndMakeVisible(modDialGain);
			addAndMakeVisible(modDialVowelA);
			addAndMakeVisible(modDialVowelB);
			addAndMakeVisible(modDialPosEnv);
			addAndMakeVisible(modDialQEnv);
			addAndMakeVisible(modDialGainEnv);

			makeKnob(PID::FormantPos, knobPos);
			makeKnob(PID::FormantQ, knobQ);
			makeKnob(PID::FormantGain, knobGain);
			makeKnob(PID::FormantA, knobVowelA);
			makeKnob(PID::FormantB, knobVowelB);
			makeKnob(PID::FormantPosEnv, knobPosEnv);
			makeKnob(PID::FormantQEnv, knobQEnv);
			makeKnob(PID::FormantGainEnv, knobGainEnv);

			modDialPos.attach(PID::FormantPos);
			modDialQ.attach(PID::FormantQ);
			modDialGain.attach(PID::FormantGain);
			modDialVowelA.attach(PID::FormantA);
			modDialVowelB.attach(PID::FormantB);
			modDialPosEnv.attach(PID::FormantPosEnv);
			modDialQEnv.attach(PID::FormantQEnv);
			modDialGainEnv.attach(PID::FormantGainEnv);

			const auto labelFont = font::dosisMedium();
			makeTextLabel(labelPos, "Pos", labelFont, Just::centred, CID::Interact);
			makeTextLabel(labelQ, "Q", labelFont, Just::centred, CID::Interact);
			makeTextLabel(labelGain, "Gain", labelFont, Just::centred, CID::Interact);
			makeTextLabel(labelVowelA, "Vowel A", labelFont, Just::centred, CID::Interact);
			makeTextLabel(labelVowelB, "Vowel B", labelFont, Just::centred, CID::Interact);
			makeTextLabel(labelPosEnv, "Env", labelFont, Just::centred, CID::Interact);
			makeTextLabel(labelQEnv, "Env", labelFont, Just::centred, CID::Interact);
			makeTextLabel(labelGainEnv, "Env", labelFont, Just::centred, CID::Interact);
			labelGroup.add(labelPos);
			labelGroup.add(labelQ);
			labelGroup.add(labelGain);
			labelGroup.add(labelVowelA);
			labelGroup.add(labelVowelB);
			labelGroup.add(labelPosEnv);
			labelGroup.add(labelQEnv);
			labelGroup.add(labelGainEnv);
		}

		void paint(Graphics& g) override
		{
			setCol(g, CID::Bg);
			g.fillAll();
		}

		void resized() override
		{
			const auto w = static_cast<float>(getWidth());
			const auto h = static_cast<float>(getHeight());
			const auto wKnob = w / 5.f;
			const auto hKnob = h * .25f;
			auto y = 0.f;
			auto x = 0.f;
			knobPos.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			knobQ.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			knobGain.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			knobVowelA.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			knobVowelB.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());

			locateAtKnob(modDialPos, knobPos);
			locateAtKnob(modDialQ, knobQ);
			locateAtKnob(modDialGain, knobGain);
			locateAtKnob(modDialVowelA, knobVowelA);
			locateAtKnob(modDialVowelB, knobVowelB);

			y += hKnob;
			x = 0.f;
			labelPos.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			labelQ.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			labelGain.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			labelVowelA.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			labelVowelB.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());

			y += hKnob;
			x = 0.f;

			knobPosEnv.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			knobQEnv.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			knobGainEnv.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());

			locateAtKnob(modDialPosEnv, knobPosEnv);
			locateAtKnob(modDialQEnv, knobQEnv);
			locateAtKnob(modDialGainEnv, knobGainEnv);

			y += hKnob;
			x = 0.f;

			labelPosEnv.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			labelQEnv.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());
			x += wKnob;
			labelGainEnv.setBounds(BoundsF(x, y, wKnob, hKnob).toNearestInt());

			labelGroup.setMaxHeight(utils.thicc);
		}

	private:
		Knob knobPos, knobQ, knobGain, knobVowelA, knobVowelB, knobPosEnv, knobQEnv, knobGainEnv;
		ModDial modDialPos, modDialQ, modDialGain, modDialVowelA, modDialVowelB, modDialPosEnv, modDialQEnv, modDialGainEnv;
		Label labelPos, labelQ, labelGain, labelVowelA, labelVowelB, labelPosEnv, labelQEnv, labelGainEnv;
		LabelGroup labelGroup;
	};
}
