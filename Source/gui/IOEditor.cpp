#include "IOEditor.h"

namespace gui
{
	// IOEditor::SidePanelParam

	IOEditor::SidePanelParam::SidePanelParam(Utils& u) :
		layout(),
		label(u),
		param(u),
		modDial(u)
	{
		layout.init
		(
			{ 2, 8, 1 },
			{ 1 }
		);
	}

	void IOEditor::SidePanelParam::init(IOEditor& editor, const String& name, PID pID)
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

	void IOEditor::SidePanelParam::setBounds(BoundsF bounds)
	{
		layout.resized(bounds);
		layout.place(label, 0, 0, 1, 1);
		layout.place(param, 1, 0, 1, 1);
		layout.place(modDial, 2, 0, 1, 1);
	}

	// IOEditor

	IOEditor::IOEditor(Utils& u) :
		Comp(u),
		titleXen(u),
		titleRefPitch(u),
		titleMasterTune(u),
		titleMacro(u),
		xen(u),
		refPitch(u),
		masterTune(u),
		macro(u),
		modDialXen(u),
		modDialRefPitch(u),
		modDialMasterTune(u),
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
		labelGroup(),
		tuningLabelGroup()
	{
		layout.init
		(
			{ 1, 1, 1 },
			{ 2, 1, 2, 2, 1, 2, 2, 2, 2, 8 }
		);

		initKnobs();
		initMacroRel();
		initMacroSwap();
		initSidePanels();
		initButtons();
		initVoiceGrid();
	}

	void IOEditor::paint(Graphics& g)
	{
		const auto bounds = getLocalBounds().toFloat();
		const auto thicc = utils.thicc;
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

	void IOEditor::resized()
	{
		layout.resized(getLocalBounds().toFloat());
		layout.place(xen, 0, 0, 1, 1);
		layout.place(refPitch, 1, 0, 1, 1);
		layout.place(masterTune, 2, 0, 1, 1);
		layout.place(titleXen, 0, 1, 1, 1);
		layout.place(titleRefPitch, 1, 1, 1, 1);
		layout.place(titleMasterTune, 2, 1, 1, 1);
		layout.place(titleMacro, 0, 3, 1, 1);
		titleMacro.setMaxHeight();
		layout.place(macro, 1, 2, 1, 2);
		layout.place(buttons[kMacroRel], 2, 2, 1, 1);
		layout.place(buttons[kMacroSwap], 2, 3, 1, 1);
		layout.place(voiceGrid, 0, 4, 3, 1);
		for (auto i = 0; i < sidePanelParams.size(); ++i)
		{
			auto& spp = sidePanelParams[i];
			const auto bounds = layout(0, 5 + i, 3, 1);
			spp.setBounds(bounds);
		}
		for (auto i = 2; i < buttons.size(); ++i)
		{
			auto& btn = buttons[i];
			layout.place(btn, i - 2, -3, 1, 1);
		}

		labelGroup.setMaxHeight();
		tuningLabelGroup.setMaxHeight();

		locateAtKnob(modDialXen, xen);
		locateAtKnob(modDialRefPitch, refPitch);
		locateAtKnob(modDialMasterTune, masterTune);
	}

	//

	void IOEditor::initMacroSwap()
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
					const auto modBias = param->getModBias();

					param->beginGesture();
					param->setValueNotifyingHost(valModMax);
					param->endGesture();
					param->setModDepth(juce::jlimit(-1.f, 1.f, val - valModMax));
					param->setModBias(1.f - modBias);
				}
			};
	}

	void IOEditor::initMacroRel()
	{
		auto& buttonMacroRel = buttons[kMacroRel];
		makeTextButton(buttonMacroRel, "Rel", "Switch between absolute and relative macro modulation.", CID::Interact);
		buttonMacroRel.type = Button::Type::kToggle;
		buttonMacroRel.onPaint = makeButtonOnPaint(false);
		buttonMacroRel.onClick = [&u = utils, &b = buttonMacroRel](const Mouse&)
			{
				u.audioProcessor.params.switchModDepthAbsolute();
				b.value = u.audioProcessor.params.isModDepthAbsolute() ? 0.f : 1.f;
				b.label.setText(b.value > .5f ? "Rel" : "Abs");
				b.label.repaint();
			};

		const auto fps = cbFPS::k15;
		const auto inc = msToInc(1000.f, fps);
		add(Callback([&, inc]()
			{
				auto& phase = callbacks[cbMacroRel].phase;
				phase += inc;
				if (phase < 1.f)
					return;
				callbacks[cbMacroRel].stop(0.f);
				buttonMacroRel.value = utils.audioProcessor.params.isModDepthAbsolute() ? 0.f : 1.f;
				buttonMacroRel.label.setText(buttonMacroRel.value > .5f ? "Rel" : "Abs");
				buttonMacroRel.label.repaint();
			}, cbMacroRel, fps, true));
	}

	void IOEditor::initKnobs()
	{
		addAndMakeVisible(titleXen);
		addAndMakeVisible(titleRefPitch);
		addAndMakeVisible(titleMasterTune);
		addAndMakeVisible(titleMacro);
		addAndMakeVisible(macro);
		addAndMakeVisible(xen);
		addAndMakeVisible(refPitch);
		addAndMakeVisible(masterTune);
		addAndMakeVisible(modDialXen);
		addAndMakeVisible(modDialRefPitch);
		addAndMakeVisible(modDialMasterTune);

		const auto fontKnobs = font::dosisRegular();

		makeTextLabel(titleXen, "Xen", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::Xen, xen);
		makeTextLabel(titleRefPitch, "Ref Pitch", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::ReferencePitch, refPitch);
		makeTextLabel(titleMasterTune, "Master Tune", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::MasterTune, masterTune);
		makeTextLabel(titleMacro, "Macro", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::Macro, macro, false);

		tuningLabelGroup.add(titleXen);
		tuningLabelGroup.add(titleRefPitch);
		tuningLabelGroup.add(titleMasterTune);

		modDialXen.attach(PID::Xen);
		modDialRefPitch.attach(PID::ReferencePitch);
		modDialMasterTune.attach(PID::MasterTune);
	}

	void IOEditor::initSidePanels()
	{
		sidePanelParams[kWet].init(*this, "Wet", PID::GainWet);
		sidePanelParams[kMix].init(*this, "Mix", PID::Mix);
		sidePanelParams[kOut].init(*this, "Out", PID::GainOut);
		for (auto& spp : sidePanelParams)
			labelGroup.add(spp.label);
	}

	void IOEditor::initVoiceGrid()
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

	void IOEditor::initButtons()
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
}