#pragma once
#include "Knob.h"
#include "VoiceGrid.h"

namespace gui
{
	struct IOEditor :
		public Comp
	{
		enum { kWet, kMix, kOut, kMacro, kSidePanelParams };
		enum { kMacroRel, kMacroSwap, kPower, kDelta, kMidSide, kNumButtons };

		struct SidePanelParam
		{
			SidePanelParam(Utils& u) :
				layout(),
				label(u),
				param(u),
				modDial(u)
			{
				layout.init
				(
					{ 1, 5, 1 },
					{ 1 }
				);
			}

			void init(IOEditor& editor, const String& name, PID pID)
			{
				editor.addAndMakeVisible(label);
				editor.addAndMakeVisible(param);
				editor.addAndMakeVisible(modDial);
				
				const auto fontKnobs = font::dosisMedium();
				const auto just = Just::centred;

				makeTextLabel(label, name, fontKnobs, just, CID::Txt);
				makeSlider(pID, param);
				modDial.attach(pID);
				modDial.verticalDrag = false;


				label.autoMaxHeight = false;
			}

			void setBounds(BoundsF bounds)
			{
				layout.resized(bounds);
				layout.place(label, 0, 0, 1, 1);
				layout.place(param, 1, 0, 1, 1);
				layout.place(modDial, 2, 0, 1, 1);
			}

			Layout layout;
			Label label;
			Knob param;
			ModDial modDial;
		};

		IOEditor(Utils& u) :
			Comp(u),
			titleMacro(u),
			macro(u),
			sidePanelParams
			{
				SidePanelParam(u),
				SidePanelParam(u),
				SidePanelParam(u),
				SidePanelParam(u)
			},
			buttons
			{
				Button(u),
				Button(u),
				Button(u),
				Button(u),
				Button(u)
			},
			voiceGrid(u),
			labelGroup()
		{
			layout.init
			(
				{ 1, 1, 1 },
				{ 2, 2, 1, 2, 2, 2, 2, 13 }
			);

			initMacro();
			initMacroRel();
			initMacroSwap();
			initSidePanels();
			initButtons();
			initVoiceGrid();
		}

		void paint(Graphics& g) override
		{
			const auto bounds = getLocalBounds().toFloat();
			const auto thicc = utils.thicc;
			//const auto col = Colours::c(CID::Darken);
			setCol(g, CID::Bg);
			Path path;
			const auto startPoint = layout(-1, 0);
			path.startNewSubPath(startPoint);
			path.lineTo(layout(0, 0));
			path.lineTo(layout(0, -2));
			path.quadraticTo(layout(-1, -2), layout(-1, -1));
			closePathOverBounds(path, bounds, startPoint, thicc, 1, 2, 1, 1);
			g.fillPath(path);
		}

		void resized()
		{
			layout.resized(getLocalBounds().toFloat());
			layout.place(titleMacro, 0, 1, 1, 1);
			titleMacro.setMaxHeight();
			layout.place(macro, 1, 0, 1, 2);
			layout.place(buttons[kMacroRel], 2, 0, 1, 1);
			layout.place(buttons[kMacroSwap], 2, 1, 1, 1);
			layout.place(voiceGrid, 0, 2, 3, 1);
			for (auto i = 0; i < sidePanelParams.size(); ++i)
			{
				auto& spp = sidePanelParams[i];
				const auto bounds = layout(0, 3 + i, 3, 1);
				spp.setBounds(bounds);
			}
			for (auto i = 2; i < buttons.size(); ++i)
			{
				auto& btn = buttons[i];
				layout.place(btn, i - 2, -3, 1, 1);
			}
			labelGroup.setMaxHeight();
		}

	protected:
		Label titleMacro;
		Knob macro;
		std::array<SidePanelParam, kSidePanelParams> sidePanelParams;
		std::array<Button, kNumButtons> buttons;
		VoiceGrid<dsp::AutoMPE::VoicesSize> voiceGrid;
		LabelGroup labelGroup;

		void initMacroSwap()
		{
			auto& buttonMacroSwap = buttons[kMacroSwap];
			buttonMacroSwap.setTooltip("Swap all parameters' main values with their modulation destinations.");
			buttonMacroSwap.onPaint = makeButtonOnPaintSwap();
			buttonMacroSwap.onClick = [&u = utils](const Mouse&)
			{
				for (auto param : u.audioProcessor.params.data())
				{
					const auto val = param->getValue();
					const auto modDepth = param->getModDepth();
					const auto valModMax = juce::jlimit(0.f, 1.f, val + modDepth);

					param->beginGesture();
					param->setValueNotifyingHost(valModMax);
					param->endGesture();
					param->setModDepth(juce::jlimit(-1.f, 1.f, val - valModMax));
				}
			};
		}

		void initMacroRel()
		{
			auto& buttonMacroRel = buttons[kMacroRel];
			String textAbsRel;
			if (utils.audioProcessor.params.isModDepthAbsolute())
			{
				buttonMacroRel.value = 0.f;
				textAbsRel = "Abs";
			}
			else
			{
				buttonMacroRel.value = 1.f;
				textAbsRel = "Rel";
			}
			makeTextButton(buttonMacroRel, textAbsRel, "Switch between absolute and relative macro modulation modes.", CID::Interact);
			buttonMacroRel.type = Button::Type::kToggle;
			buttonMacroRel.onPaint = makeButtonOnPaint(false);
			buttonMacroRel.onClick = [&u = utils, &b = buttonMacroRel](const Mouse&)
			{
				u.audioProcessor.params.switchModDepthAbsolute();
				b.value = u.audioProcessor.params.isModDepthAbsolute() ? 0.f : 1.f;
				b.label.setText(b.value > .5f ? "Rel" : "Abs");
				b.label.repaint();
			};
		}

		void initMacro()
		{
			const auto fontKnobs = font::dosisRegular();
			makeTextLabel(titleMacro, "Macro", fontKnobs, Just::centred, CID::Txt);
			makeKnob(PID::Macro, macro, false);
			addAndMakeVisible(macro);
			addAndMakeVisible(titleMacro);
		}

		void initSidePanels()
		{
			sidePanelParams[kWet].init(*this, "Wet", PID::GainWet);
			sidePanelParams[kMix].init(*this, "Mix", PID::Mix);
			sidePanelParams[kOut].init(*this, "Out", PID::GainOut);
			for (auto& spp : sidePanelParams)
				labelGroup.add(spp.label);
		}

		void initVoiceGrid()
		{
			addAndMakeVisible(voiceGrid);
			voiceGrid.init([&](VoiceGrid<dsp::AutoMPE::VoicesSize>::Voices& voices)
			{
				bool updated = false;
				const auto& pp = utils.audioProcessor.pluginProcessor.parallelProcessor;
				for (auto i = 0; i < voices.size(); ++i)
				{
					const auto active = !pp.isSleepy(i);
					if (voices[i] != active)
					{
						updated = true;
						voices[i] = active;
					}
				}
				return updated;
			});
		}

		void initButtons()
		{
			for (auto& btn : buttons)
				addAndMakeVisible(btn);
			// POWER BUTTON
			auto& powerButton = buttons[kPower];
			makeParameter(powerButton, PID::Power, Button::Type::kToggle, makeButtonOnPaintPower());
			const auto powerButtonOnClick = powerButton.onClick;
			powerButton.onClick = [&, oc = powerButtonOnClick](const Mouse& mouse)
			{
				oc(mouse);
				powerButton.value = std::round(utils.audioProcessor.params(PID::Power).getValMod());
				repaint();
			};
			powerButton.type = Button::Type::kToggle;
			powerButton.value = std::round(utils.audioProcessor.params(PID::Power).getValMod());
			// DELTA BUTTON
			auto& deltaButton = buttons[kDelta];
			makeParameter(deltaButton, PID::Delta, Button::Type::kToggle, makeButtonOnPaintPolarity());
			deltaButton.type = Button::Type::kToggle;
			deltaButton.value = std::round(utils.audioProcessor.params(PID::Delta).getValMod());
			// MID/SIDE BUTTON
			auto& midSideButton = buttons[kMidSide];
			makeParameter(midSideButton, PID::StereoConfig, Button::Type::kChoice, "L/R;M/S");
		}
	};
}