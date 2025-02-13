#include "TopEditor.h"

namespace gui
{
	TopEditor::TopEditor(Utils& u, patch::Browser& browser) :
		Comp(u),
		patchBrowserButton(u, browser),
		buttonRandomizer(u, "randall"),
		buttonSoftClip(u)
	{
		layout.init
		(
			{ 2, 1, 1 },
			{ 5, 1 }
		);

		addAndMakeVisible(patchBrowserButton);
		addAndMakeVisible(buttonRandomizer);
		addAndMakeVisible(buttonSoftClip);

		makeParameter(buttonSoftClip, PID::SoftClip, Button::Type::kToggle, makeButtonOnPaintClip());
		for (auto i = 0; i < param::NumParams; ++i)
		{
			const auto pID = static_cast<PID>(i);
			if (pID != PID::KeySelectorEnabled && pID != PID::NoiseBlend)
				if (pID != PID::EnvGenAmpAttack && pID != PID::EnvGenAmpDecay && pID != PID::EnvGenAmpSustain && pID != PID::EnvGenAmpRelease
					&& pID != PID::EnvGenModAttack && pID != PID::EnvGenModDecay && pID != PID::EnvGenModSustain && pID != PID::EnvGenModRelease)
				buttonRandomizer.add(pID);
		}

		const auto randModalFunc = [&](ButtonRandomizer::RandomSeed& rand)
		{
			auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
			for (auto i = 0; i < 2; ++i)
				modalFilter.randomizeMaterial(rand, i);
		};

		buttonRandomizer.add(randModalFunc);

		const auto enabledSoftclipFunc = [&](ButtonRandomizer::RandomSeed&)
		{
			auto& param = utils.audioProcessor.params(PID::SoftClip);
			param.setValueWithGesture(1.f);
		};

		buttonRandomizer.add(enabledSoftclipFunc);

		const auto disableXenSnapFunc = [&](ButtonRandomizer::RandomSeed&)
		{
			auto& param = utils.audioProcessor.params(PID::XenSnap);
			param.setValueWithGesture(0.f);
		};

		buttonRandomizer.add(disableXenSnapFunc);
	}

	void TopEditor::resized()
	{
		layout.resized(getLocalBounds());
		layout.place(patchBrowserButton, 0, 0, 1, 1);
		layout.place(buttonSoftClip, 1, 0, 1, 1);
		layout.place(buttonRandomizer, 2, 0, 1, 1);
	}

	void TopEditor::paint(Graphics& g)
	{
		const auto bottom = layout.bottom();
		setCol(g, CID::Bg);
		const auto thicc = utils.thicc;
		const auto thiccHalf = thicc * .5f;
		const auto thicc2 = thicc * 2;
		g.fillRoundedRectangle(bottom.reduced(thiccHalf).withX(bottom.getX() + thicc2), thicc);
	}
}