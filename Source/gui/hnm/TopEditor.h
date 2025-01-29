#pragma once
#include "../ButtonRandomizer.h"
#include "../PatchBrowser.h"

namespace gui
{
	struct TopEditor :
		public Comp
	{
		TopEditor(Utils& u, patch::Browser& browser) :
			Comp(u),
			patchBrowserButton(u, browser),
			titleRandomizer(u),
			titleRandomizerNoPitch(u),
			titleRandomizerNoEnv(u),
			titleRandomizerNoPitchEnv(u),
			buttonRandomizer(u, "randomizer"),
			buttonRandomizerNoPitch(u, "randomizerNoPitch"),
			buttonRandomizerNoEnv(u, "randomizerNoEnv"),
			buttonRandomizerNoPitchEnv(u, "randomizerNoPitchEnv"),
			buttonSoftClip(u)
		{
			layout.init
			(
				{ 2, 1, 1, 1, 1, 1 },
				{ 2, 5 }
			);

			addAndMakeVisible(patchBrowserButton);
			addAndMakeVisible(buttonRandomizer);
			addAndMakeVisible(buttonRandomizerNoPitch);
			addAndMakeVisible(buttonRandomizerNoEnv);
			addAndMakeVisible(buttonRandomizerNoPitchEnv);
			addAndMakeVisible(buttonSoftClip);
			addAndMakeVisible(titleRandomizer);
			addAndMakeVisible(titleRandomizerNoPitch);
			addAndMakeVisible(titleRandomizerNoEnv);
			addAndMakeVisible(titleRandomizerNoPitchEnv);

			const auto font = font::flx();
			const auto just = Just::centredLeft;
			makeTextLabel(titleRandomizer, "Rand All:", font, just, CID::Txt);
			makeTextLabel(titleRandomizerNoPitch, "Rand -Pitch:", font, just, CID::Txt);
			makeTextLabel(titleRandomizerNoEnv, "Rand -Env:", font, just, CID::Txt);
			makeTextLabel(titleRandomizerNoPitchEnv, "Rand Only Modal:", font, just, CID::Txt);
			titleRandomizer.autoMaxHeight = true;
			titleRandomizerNoPitch.autoMaxHeight = true;
			titleRandomizerNoEnv.autoMaxHeight = true;
			titleRandomizerNoPitchEnv.autoMaxHeight = true;

			makeParameter(buttonSoftClip, PID::SoftClip, Button::Type::kToggle, makeButtonOnPaintClip());
			for (auto i = 0; i < param::NumParams; ++i)
			{
				const auto pID = static_cast<PID>(i);
				if (pID != PID::KeySelectorEnabled && pID != PID::NoiseBlend)
				{
					buttonRandomizer.add(pID);

					const bool noPitch = pID != PID::AnchorPitch && pID != PID::CombOct && pID != PID::CombSemi &&
						pID != PID::MasterTune && pID != PID::Xen && pID != PID::PitchbendRange &&
						pID != PID::ModalOct && pID != PID::ModalSemi;

					const bool noEnv = pID != PID::EnvGenModAttack && pID != PID::EnvGenModDecay &&
						pID != PID::EnvGenModSustain && pID != PID::EnvGenModRelease &&
						pID != PID::EnvGenAmpAttack && pID != PID::EnvGenAmpDecay &&
						pID != PID::EnvGenAmpSustain && pID != PID::EnvGenAmpRelease;

					if (noPitch)
						buttonRandomizerNoPitch.add(pID);
					if(noEnv)
						buttonRandomizerNoEnv.add(pID);
					if (noPitch && noEnv)
						buttonRandomizerNoPitchEnv.add(pID);
				}
			}

			const auto randModalFunc = [&](ButtonRandomizer::Randomizer& rand)
			{
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& xenManager = utils.audioProcessor.xenManager;

				for (auto i = 0; i < 2; ++i)
				{
					auto& mat = modalFilter.getMaterial(i);
					if (mat.status == dsp::modal::StatusMat::Processing)
					{
						std::array<double, dsp::modal::NumFilters> ratios;
						ratios[0] = 1.;
						for (auto j = 1; j < dsp::modal::NumFilters; ++j)
							ratios[j] = 1. + static_cast<double>(rand()) * 32.;
						std::sort(ratios.begin(), ratios.end());

						auto& peaks = mat.peakInfos;

						peaks[0].mag = static_cast<double>(rand());
						peaks[0].ratio = ratios[0];
						peaks[0].freqHz = xenManager.noteToFreqHz(static_cast<double>(rand()) * 127.);
						peaks[0].keytrack = 0.;

						for (auto j = 1; j < dsp::modal::NumFilters; ++j)
						{
							auto& peak = peaks[j];
							peak.mag = static_cast<double>(rand());
							peak.ratio = ratios[j];
							peak.freqHz = xenManager.noteToFreqHz(static_cast<double>(rand()) * 127.);
							peak.keytrack = static_cast<double>(rand());
						}

						mat.reportUpdate();
					}
				}
			};

			buttonRandomizer.add(randModalFunc);
			buttonRandomizerNoPitch.add(randModalFunc);
			buttonRandomizerNoEnv.add(randModalFunc);
			buttonRandomizerNoPitchEnv.add(randModalFunc);

			const auto enabledSoftclipFunc = [&](ButtonRandomizer::Randomizer&)
			{
				auto& param = utils.audioProcessor.params(PID::SoftClip);
				param.setValueWithGesture(1.f);
			};

			buttonRandomizer.add(enabledSoftclipFunc);

			const auto disableXenSnapFunc = [&](ButtonRandomizer::Randomizer&)
			{
				auto& param = utils.audioProcessor.params(PID::XenSnap);
				param.setValueWithGesture(0.f);
			};

			buttonRandomizer.add(disableXenSnapFunc);
			buttonRandomizerNoEnv.add(disableXenSnapFunc);
		}

		void resized() override
		{
			layout.resized(getLocalBounds());
			layout.place(patchBrowserButton, 0, 1, 1, 1);
			layout.place(titleRandomizer, 2, 0.f, 1, 1.2f);
			layout.place(titleRandomizerNoPitch, 3, 0.f, 1, 1.2f);
			layout.place(titleRandomizerNoEnv, 4, 0.f, 1, 1.2f);
			layout.place(titleRandomizerNoPitchEnv, 5, 0.f, 1, 1.2f);
			layout.place(buttonSoftClip, 1, 1, 1, 1);
			layout.place(buttonRandomizer, 2, 1, 1, 1);
			layout.place(buttonRandomizerNoPitch, 3, 1, 1, 1);
			layout.place(buttonRandomizerNoEnv, 4, 1, 1, 1);
			layout.place(buttonRandomizerNoPitchEnv, 5, 1, 1, 1);
		}

		void paint(Graphics& g) override
		{
			const auto randArea = layout(2, 0, 4, 2);
			setCol(g, CID::Darken);
			g.fillRect(randArea);
		}

	protected:
		patch::BrowserButton patchBrowserButton;
		Label titleRandomizer, titleRandomizerNoPitch, titleRandomizerNoEnv, titleRandomizerNoPitchEnv;
		ButtonRandomizer buttonRandomizer, buttonRandomizerNoPitch, buttonRandomizerNoEnv, buttonRandomizerNoPitchEnv;
		Button buttonSoftClip;
	};
}