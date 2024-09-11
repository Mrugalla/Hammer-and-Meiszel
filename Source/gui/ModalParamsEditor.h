#pragma once
#include "../audio/dsp/modal2/Axiom.h"
#include "Knob.h"
#include "Button.h"

namespace gui
{
	struct ModalMainParams :
		public Comp
	{
		using kParam = dsp::modal2::kParam;

		ModalMainParams(Utils& u, PID smartKeytrack, PID blend,
			PID spread, PID harmonie, PID kraft, PID reso) :
			Comp(u),
			knobs{ Knob(u), Knob(u), Knob(u), Knob(u), Knob(u), Knob(u) },
			modDials{ ModDial(u), ModDial(u), ModDial(u), ModDial(u), ModDial(u), ModDial(u) }
		{
			for(auto& knob: knobs)
				addAndMakeVisible(knob);
			for(auto& modDial : modDials)
				addAndMakeVisible(modDial);
			
			makeKnob(smartKeytrack, knobs[kParam::kSmartKeytrack]);
			makeKnob(blend, knobs[kParam::kBlend]);
			makeKnob(spread, knobs[kParam::kSpreizung]);
			makeKnob(harmonie, knobs[kParam::kHarmonie]);
			makeKnob(kraft, knobs[kParam::kKraft]);
			makeKnob(reso, knobs[kParam::kReso]);

			modDials[kParam::kSmartKeytrack].attach(smartKeytrack);
			modDials[kParam::kBlend].attach(blend);
			modDials[kParam::kSpreizung].attach(spread);
			modDials[kParam::kHarmonie].attach(harmonie);
			modDials[kParam::kKraft].attach(kraft);
			modDials[kParam::kReso].attach(reso);
		}

		void resized() override
		{
			const auto w = static_cast<float>(getWidth());
			const auto h = static_cast<float>(getHeight());
			const auto y = 0.f;
			const auto wKnob = w / static_cast<float>(knobs.size());
			auto x = 0.f;
			for (auto& knob : knobs)
			{
				knob.setBounds(BoundsF(x, y, wKnob, h).toNearestInt());
				x += wKnob;
			}

			for (auto i = 0; i < kParam::kNumParams; ++i)
				locateAtKnob(modDials[i], knobs[i]);
		}

	protected:
		std::array<Knob, dsp::modal2::kNumParams> knobs;
		std::array<ModDial, dsp::modal2::kNumParams> modDials;
	};

	struct ModalParamsEditor :
		public Comp
	{
		using kParam = dsp::modal2::kParam;
		enum kTab { kMain, kEnv, kWidth, kNumTabs };
		
		struct OctSemiSlider :
			public Comp
		{
			OctSemiSlider(Utils& u, PID pID, String&& name) :
				Comp(u),
				label(u),
				knob(u),
				modDial(u)
			{
				layout.init
				(
					{ 1, 3, 1 },
					{ 1 }
				);

				addAndMakeVisible(label);
				addAndMakeVisible(knob);
				addAndMakeVisible(modDial);
				makeSlider(pID, knob);
				modDial.attach(pID);
				makeTextLabel(label, name, font::dosisBold(), Just::centredRight, CID::Txt);
				modDial.verticalDrag = false;
			}

			void resized() override
			{
				layout.resized(getLocalBounds());
				layout.place(label, 0, 0, 1, 1);
				layout.place(knob, 1, 0, 1, 1);
				locateAtSlider(modDial, knob);
			}

			Label label;
			Knob knob;
			ModDial modDial;
		};

		struct TabButtons :
			public Comp
		{
			enum kAnis { kAniTabChange, kNumAnis };

			TabButtons(Utils& u, int defaultTab) :
				Comp(u),
				buttons{ Button(u), Button(u), Button(u) },
				labelGroup(),
				tabLast(defaultTab),
				tabCur(defaultTab)
			{
				makeTextButton(buttons[kTab::kMain], "Main", "Click here to adjust the parameters of the modal filter!", CID::Interact);
				makeTextButton(buttons[kTab::kEnv], "Env", "Click here to adjust the envelope depths of the modal filter!", CID::Interact);
				makeTextButton(buttons[kTab::kWidth], "Width", "Click here to adjust the stereo width of the modal filter!", CID::Interact);

				for (auto& button : buttons)
				{
					addAndMakeVisible(button);
					button.label.autoMaxHeight = false;
					labelGroup.add(button.label);
				}
				
				for (auto& button : buttons)
				{
					button.type = Button::Type::kToggle;
					button.value = 0.f;
				}
				buttons[defaultTab].value = 1.f;

				for (auto i = 0; i < kNumTabs; ++i)
				{
					buttons[i].onClick = [this, i](const Mouse&)
					{
						if (buttons[i].value > .5f)
							return;

						for (auto& tabButton : buttons)
							tabButton.value = 0.f;
						buttons[i].value = 1.f;
						for (auto& tabButton : buttons)
							tabButton.repaint();

						tabCur = i;
						callbacks[kAniTabChange].start(0.f);
					};
				}

				const auto fps = cbFPS::k60;
				const auto speed = msToInc(AniLengthMs, fps);

				add(Callback([&, speed]()
				{
					auto& phase = callbacks[kAniTabChange].phase;
					phase += speed;
					if (phase >= 1.f)
					{
						phase = 1.f;
						callbacks[kAniTabChange].active = false;
						for (auto i = 0; i < kNumTabs; ++i)
							if (buttons[i].value > .5f)
								tabLast = i;
					}
					getParentComponent()->resized();
				}, kAniTabChange, fps, false));
			}

			void resized() override
			{
				const auto area = getLocalBounds().toFloat();
				const auto x = 0.f;
				const auto w = area.getWidth();
				const auto h = area.getHeight();
				const auto hButton = h / static_cast<float>(kNumTabs);
				auto y = area.getY();
				for (auto& button : buttons)
				{
					button.setBounds(BoundsF(x, y, w, hButton).toNearestInt());
					y += hButton;
				}
				labelGroup.setMaxHeight(utils.thicc);
			}

			float getAnimationPhase() const noexcept
			{
				return callbacks[kAniTabChange].phase;
			}

		protected:
			std::array<Button, kNumTabs> buttons;
			LabelGroup labelGroup;
		public:
			int tabLast, tabCur;
		};

		ModalParamsEditor(Utils& u) :
			Comp(u),
			tabButtons(u, 0),
			params
			{
				ModalMainParams(u, PID::ModalSmartKeytrack, PID::ModalBlend,
					PID::ModalSpreizung, PID::ModalHarmonie, PID::ModalKraft, PID::ModalResonanz),
				ModalMainParams(u, PID::ModalSmartKeytrackEnv, PID::ModalBlendEnv,
					PID::ModalSpreizungEnv, PID::ModalHarmonieEnv, PID::ModalKraftEnv, PID::ModalResonanzEnv),
				ModalMainParams(u, PID::ModalSmartKeytrackBreite, PID::ModalBlendBreite,
					PID::ModalSpreizungBreite, PID::ModalHarmonieBreite, PID::ModalKraftBreite, PID::ModalResonanzBreite)
			},
			oct(u, PID::ModalOct, "Oct"),
			semi(u, PID::ModalSemi, "Semi"),
			labels{ Label(u), Label(u), Label(u), Label(u), Label(u), Label(u) },
			labelGroup(),
			octSemiGroup()
		{
			layout.init
			(
				{ 1, 1, 1, 1, 1, 1, 1 },
				{ 3, 13, 2 }
			);

			addAndMakeVisible(oct);
			addAndMakeVisible(semi);
			for (auto& label : labels)
				addAndMakeVisible(label);
			for (auto& p : params)
				addAndMakeVisible(p);
			addAndMakeVisible(tabButtons);

			const auto fontKnobs = font::dosisBold();
			const auto just = Just::centred;

			makeTextLabel(labels[kParam::kSmartKeytrack], "Keytrack+", fontKnobs, just, CID::Txt);
			makeTextLabel(labels[kParam::kBlend], "Blend", fontKnobs, just, CID::Txt);
			makeTextLabel(labels[kParam::kSpreizung], "Spreizung", fontKnobs, just, CID::Txt);
			makeTextLabel(labels[kParam::kHarmonie], "Harmonie", fontKnobs, just, CID::Txt);
			makeTextLabel(labels[kParam::kKraft], "Kraft", fontKnobs, just, CID::Txt);
			makeTextLabel(labels[kParam::kReso], "Reso", fontKnobs, just, CID::Txt);

			for (auto& label : labels)
				labelGroup.add(label);
			octSemiGroup.add(oct.label);
			octSemiGroup.add(semi.label);
		}

		void resized() override
		{
			layout.resized(getLocalBounds());
			layout.place(oct, 0, 0, 2, 1);
			layout.place(semi, 2, 0, 2, 1);
			layout.place(tabButtons, 6, 1, 1, 2);
			
			{
				const auto area = layout(0, 1, 6, 1).toNearestInt();
				const auto w = area.getWidth();
				const auto wKnob = w / static_cast<float>(kParam::kNumParams);

				const auto phase = tabButtons.getAnimationPhase();
				const auto xLast = w * tabButtons.tabLast + tabButtons.tabLast * wKnob;
				const auto xCur = w * tabButtons.tabCur + tabButtons.tabCur * wKnob;
				const auto xNow = xLast + phase * (xCur - xLast);

				const auto wInc = w + wKnob;
				auto xPos = 0.f;
				for (auto i = 0; i < kNumTabs; ++i)
				{
					params[i].setBounds(area.withX(static_cast<int>(xPos - xNow)));
					xPos += wInc;
				}
			}
			for (auto i = 0; i < kParam::kNumParams; ++i)
				layout.place(labels[i], i, 2, 1, 1);
			labelGroup.setMaxHeight();
			octSemiGroup.setMaxHeight();
		}

	protected:
		TabButtons tabButtons;
		std::array<ModalMainParams, kNumTabs> params;
		OctSemiSlider oct, semi;
		std::array<Label, dsp::modal2::kNumParams> labels;
		LabelGroup labelGroup;
		LabelGroup octSemiGroup;
	};
}