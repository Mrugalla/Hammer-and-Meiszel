#include "TopEditor.h"

namespace gui
{
	TopEditor::TopEditor(Utils& u, patch::Browser& browser) :
		Comp(u),
		patchBrowserButton(u, browser),
		titleRandomizer(u),
		buttonRandomizer(u, "randall"),
		buttonSoftClip(u)
	{
		layout.init
		(
			{ 2, 1, 1 },
			{ 2, 5 }
		);

		addAndMakeVisible(patchBrowserButton);
		addAndMakeVisible(buttonRandomizer);
		addAndMakeVisible(buttonSoftClip);
		addAndMakeVisible(titleRandomizer);

		const auto font = font::flx();
		const auto just = Just::centredLeft;
		makeTextLabel(titleRandomizer, "Rand All:", font, just, CID::Txt);
		titleRandomizer.autoMaxHeight = true;

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
			auto& xenManager = utils.audioProcessor.xenManager;
			for (auto i = 0; i < 2; ++i)
				modalFilter.randomizeMaterial(rand, xenManager, i);
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
		layout.place(patchBrowserButton, 0, 1, 1, 1);
		layout.place(titleRandomizer, 2, 0.f, 1, 1.2f);
		layout.place(buttonSoftClip, 1, 1, 1, 1);
		layout.place(buttonRandomizer, 2, 1, 1, 1);
	}

	void TopEditor::paint(Graphics& g)
	{
		const auto randArea = layout(2, 0, 1, 2);
		setCol(g, CID::Darken);
		g.fillRect(randArea);
	}
}