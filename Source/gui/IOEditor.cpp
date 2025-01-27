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
			{ 1, 13, 2 },
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
		layout.place(label, 0.f, 0, 1.3f, 1);
		layout.place(param, 1, 0, 1, 1);
		layout.place(modDial, 2, 0, 1, 1);
	}

	// IOEditor

	IOEditor::IOEditor(Utils& u) :
		Comp(u),
		titleXen(u),
		titleAnchor(u),
		titleMasterTune(u),
		titlePitchbend(u),
		titleMacro(u),
		xen(u),
		anchor(u),
		masterTune(u),
		pitchbend(u),
		macro(u),
		modDialXen(u),
		modDialRefPitch(u),
		modDialMasterTune(u),
		modDialPitchbend(u),
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
#if PPDHasDelta
			Button(u),
#endif
			Button(u),
			Button(u)
		},
		buttonPower(u),
		voiceGrid(u),
		labelGroup(),
		tuningLabelGroup()
	{
		layout.init
		(
			{ 1, 1, 1 },
			{ 5, 3, 2, 1, 3, 3, 3, 2, 13 }
		);

		initKnobs();
		initMacroRel();
		initMacroSwap();
		initSidePanels();
		initButtons();
		initVoiceGrid();

		setInterceptsMouseClicks(false, true);
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
		const auto tuningArea = layout(0, 0, 3, 1);
		{
			const auto y = tuningArea.getY();
			const auto w = tuningArea.getWidth();
			const auto h = tuningArea.getHeight();
			const auto w1 = w * .25f;
			const auto h1 = h * .6667f;
			const auto y1 = y + h1;
			const auto h2 = h * .3333f;
			auto x = tuningArea.getX();
			xen.setBounds(BoundsF(x, y, w1, h1).toNearestInt());
			titleXen.setBounds(BoundsF(x, y1, w1, h2).toNearestInt());
			buttons[kXenSnap].setBounds(BoundsF(x, y1 + h2, w1, h2).toNearestInt());
			x += w1;
			anchor.setBounds(BoundsF(x, y, w1, h1).toNearestInt());
			titleAnchor.setBounds(BoundsF(x, y1, w1, h2).toNearestInt());
			x += w1;
			masterTune.setBounds(BoundsF(x, y, w1, h1).toNearestInt());
			titleMasterTune.setBounds(BoundsF(x, y1, w1, h2).toNearestInt());
			x += w1;
			pitchbend.setBounds(BoundsF(x, y, w1, h1).toNearestInt());
			titlePitchbend.setBounds(BoundsF(x, y1, w1, h2).toNearestInt());

			locateAtKnob(modDialXen, xen);
			locateAtKnob(modDialRefPitch, anchor);
			locateAtKnob(modDialMasterTune, masterTune);
			locateAtKnob(modDialPitchbend, pitchbend);
		}
		layout.place(titleMacro, .2f, 2, .8f, 1);
		titleMacro.setMaxHeight();
		layout.place(macro, 1.f, 1, 1.2f, 2);
		layout.place(buttons[kMacroRel], 2.2f, 1.333f, .8f, .667f);
		layout.place(buttons[kMacroSwap], 2.2f, 2, .8f, 1);
		layout.place(voiceGrid, 0, 3, 3, 1);
		for (auto i = 0; i < sidePanelParams.size(); ++i)
		{
			auto& spp = sidePanelParams[i];
			const auto bounds = layout(0, 4 + i, 3, 1);
			spp.setBounds(bounds);
		}
		layout.place(buttonPower, 0, -3, 1, 1);
		const auto buttonMax = buttons.size() - 1;
		for (auto i = 2; i < buttonMax; ++i)
		{
			auto& btn = buttons[i];
			layout.place(btn, i - 1, -3, 1, 1);
		}

		labelGroup.setMaxHeight();
		tuningLabelGroup.setMaxHeight();
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
		makePaintButton(buttonMacroRel, [](Graphics& g, const Button& b)
		{
			const auto thicc = b.utils.thicc;
			const auto thiccHalf = thicc * .5f;
			const auto thicc2 = thicc * 2.f;
			const auto thicc3 = thicc * 3.f;
			const auto thicc5 = thicc * 5.f;

			const auto bounds = b.getLocalBounds().toFloat().reduced(thicc);
				
			const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
			const auto togglePhase = b.callbacks[Button::kToggleStateCB].phase;

			const auto bgColour = getColour(CID::Bg).interpolatedWith(getColour(CID::Interact), togglePhase * .3f);
			g.setColour(bgColour);
			const auto lineThicc = thicc + hoverPhase * thiccHalf;
			const auto boundsBg = bounds.reduced((1.f - hoverPhase) * bounds.getWidth() * .5f);
			g.fillRoundedRectangle(boundsBg, lineThicc);

			setCol(g, CID::Interact);

			const auto aWidth = togglePhase * bounds.getWidth() * .6f;
			const auto aX0 = bounds.getX();
			const auto aX1 = aX0 + aWidth;
			if (togglePhase > 0.f)
			{
				const auto aY = bounds.getY() + bounds.getHeight() * .5f;
				const LineF arrow(aX0, aY, aX1, aY);
				const auto arrowStuff = thicc5 + (1.f - togglePhase) * thicc3;
				g.drawArrow(arrow, lineThicc, arrowStuff, arrowStuff);
			}

			const auto cX = aX1;
			const auto cY = bounds.getY();
			const auto cW = bounds.getWidth() - aWidth;
			const auto cH = bounds.getHeight();
			const auto circle = maxQuadIn(BoundsF(cX, cY, cW, cH)).reduced(thicc3);
			g.drawEllipse(circle, lineThicc);

		}, "If enabled, macro modulation depth is relative to the main value.");
		buttonMacroRel.type = Button::Type::kToggle;
		buttonMacroRel.onClick = [&u = utils, &b = buttonMacroRel](const Mouse&)
		{
			u.audioProcessor.params.switchModDepthAbsolute();
			b.value = u.audioProcessor.params.isModDepthAbsolute() ? 0.f : 1.f;
			b.label.setText(b.value > .5f ? "Rel" : "Abs");
			b.label.repaint();
		};
		buttonMacroRel.value = utils.audioProcessor.params.isModDepthAbsolute() ? 0.f : 1.f;

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
		addAndMakeVisible(titleAnchor);
		addAndMakeVisible(titleMasterTune);
		addAndMakeVisible(titlePitchbend);
		addAndMakeVisible(titleMacro);
		addAndMakeVisible(macro);
		addAndMakeVisible(xen);
		addAndMakeVisible(anchor);
		addAndMakeVisible(masterTune);
		addAndMakeVisible(pitchbend);
		addAndMakeVisible(modDialXen);
		addAndMakeVisible(modDialRefPitch);
		addAndMakeVisible(modDialMasterTune);
		addAndMakeVisible(modDialPitchbend);

		const auto fontKnobs = font::dosisRegular();

		makeTextLabel(titleXen, "Xen", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::Xen, xen);
		makeTextLabel(titleAnchor, "Anchor", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::AnchorPitch, anchor);
		makeTextLabel(titleMasterTune, "Master Tune", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::PitchbendRange, pitchbend);
		makeTextLabel(titlePitchbend, "PB Range", fontKnobs, Just::centred, CID::Txt);
		makeKnob(PID::MasterTune, masterTune);
		makeTextLabel(titleMacro, "Macro", font::flx(), Just::centred, CID::Txt);
		makeKnob(PID::Macro, macro, false);

		tuningLabelGroup.add(titleXen);
		tuningLabelGroup.add(titleAnchor);
		tuningLabelGroup.add(titleMasterTune);
		tuningLabelGroup.add(titlePitchbend);

		modDialXen.attach(PID::Xen);
		modDialRefPitch.attach(PID::AnchorPitch);
		modDialMasterTune.attach(PID::MasterTune);
		modDialPitchbend.attach(PID::PitchbendRange);
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
		addAndMakeVisible(buttonPower);
#if PPDHasDelta
		// DELTA BUTTON
		auto& deltaButton = buttons[kDelta];
		makeParameter(deltaButton, PID::Delta, Button::Type::kToggle, makeButtonOnPaintPolarity());
		deltaButton.type = Button::Type::kToggle;
		deltaButton.value = std::round(utils.audioProcessor.params(PID::Delta).getValMod());
#endif
		// MID/SIDE BUTTON
		auto& midSideButton = buttons[kMidSide];
		makeParameter(midSideButton, PID::StereoConfig, Button::Type::kChoice, "L / R;M / S");
		// XEN SNAP BUTTON
		auto& xenSnapButton = buttons[kXenSnap];
		makeParameter(xenSnapButton, PID::XenSnap, Button::Type::kToggle, "x;x");
	}
}