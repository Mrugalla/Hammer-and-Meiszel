#pragma once
#include "../Knob.h"

namespace gui
{
	struct ModalPartialsFixedEditor :
		public Comp
	{
		ModalPartialsFixedEditor(Utils& u) :
			Comp(u),
			knobPos(u),
			knobQ(u),
			knobGain(u),
			knobVowelA(u),
			knobVowelB(u)
		{
			addAndMakeVisible(knobPos);
			addAndMakeVisible(knobQ);
			addAndMakeVisible(knobGain);
			addAndMakeVisible(knobVowelA);
			addAndMakeVisible(knobVowelB);

			makeKnob(PID::FormantPos, knobPos);
			makeKnob(PID::FormantQ, knobQ);
			makeKnob(PID::FormantGain, knobGain);
			makeKnob(PID::FormantA, knobVowelA);
			makeKnob(PID::FormantB, knobVowelB);
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
			auto x = 0.f;
			knobPos.setBounds(BoundsF(x, 0.f, wKnob, h).toNearestInt());
			x += wKnob;
			knobQ.setBounds(BoundsF(x, 0.f, wKnob, h).toNearestInt());
			x += wKnob;
			knobGain.setBounds(BoundsF(x, 0.f, wKnob, h).toNearestInt());
			x += wKnob;
			knobVowelA.setBounds(BoundsF(x, 0.f, wKnob, h).toNearestInt());
			x += wKnob;
			knobVowelB.setBounds(BoundsF(x, 0.f, wKnob, h).toNearestInt());
		}

	private:
		Knob knobPos, knobQ, knobGain, knobVowelA, knobVowelB;
	};
}
