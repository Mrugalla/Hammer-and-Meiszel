#include "Editor.h"

namespace gui
{
    evt::Evt makeEvt(Component& comp)
    {
        return [&c = comp](evt::Type type, const void*)
        {
            switch (type)
            {
            case evt::Type::ColourSchemeChanged:
                return repaintWithChildren(&c);
            case evt::Type::ClickedEmpty:
                return c.giveAwayKeyboardFocus();
            }
        };
    }

	Label& Editor::get(kLabels i) noexcept
    {
		return labels[static_cast<int>(i)];
	}

    Knob& Editor::get(kKnobs i) noexcept
    {
		return knobs[static_cast<int>(i)];
    }

    ModDial& Editor::getModDial(kKnobs i) noexcept
    {
		return modDials[static_cast<int>(i)];
    }

    Button& Editor::get(kButtons i) noexcept
    {
		return buttons[static_cast<int>(i)];
    }

    Editor::Editor(Processor& p) :
        AudioProcessorEditor(&p),
        audioProcessor(p),
        utils(*this, p),
        layout(),
        evtMember(utils.eventSystem, makeEvt(*this)),
        tooltip(utils),
        genAni(utils),
        toast(utils),
        labels
        {
            Label(utils),
            Label(utils),
            Label(utils),
            Label(utils),
            Label(utils),
			Label(utils),
			Label(utils),
			Label(utils),
			Label(utils),
            Label(utils),
            Label(utils),
            Label(utils),
			Label(utils),
			Label(utils)
        },
        knobs
        {
            Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils),
			Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils),
            Knob(utils)
        },
        modDials
		{
			ModDial(utils),
			ModDial(utils),
			ModDial(utils),
			ModDial(utils),
			ModDial(utils),
            ModDial(utils),
            ModDial(utils),
            ModDial(utils),
			ModDial(utils),
            ModDial(utils),
            ModDial(utils),
            ModDial(utils),
            ModDial(utils),
            ModDial(utils)
		},
        buttons
        {
            Button(utils),
			Button(utils),
            Button(utils),
            Button(utils),
            Button(utils)
        },
        materialViews
        {
            ModalMaterialView(utils, utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(0)),
            ModalMaterialView(utils, utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(1))
        },
        materialDropDown(utils)
    {
        layout.initGrid(31, 18);
        
        addAndMakeVisible(genAni);
        addAndMakeVisible(tooltip);

        String dev(JucePlugin_Manufacturer);
        auto devFont = font::flx();
        devFont.setExtraKerningFactor(.1f);
        makeTextLabel(get(kLabels::kDev), "- " + dev + " -", devFont, Just::centred, CID::Txt);

		makeTextLabel(get(kLabels::kTitleModal), "// Modal //", devFont, Just::centredLeft, CID::Txt);
		makeTextLabel(get(kLabels::kTitleFlanger), "// Flanger //", devFont, Just::centredLeft, CID::Txt);

        auto sz = String(juce::CharPointer_UTF8("\xc3\x9f"));
        String name(JucePlugin_Name);
        for (auto i = 0; i < name.length(); ++i)
            if (name[i] == 's' && name[i + 1] == 'z')
                name = name.replaceSection(i, 2, sz);
            //else if(name[i] == ' ')
            //    name = name.replaceSection(i, 1, "\n");
        auto titleFont = font::flx();
        makeTextLabel(get(kLabels::kTitle), name, titleFont, Just::centred, CID::Txt);
        for (auto& label : labels)
            addAndMakeVisible(label);

        for(auto& knob: knobs)
            addAndMakeVisible(knob);
        for (auto& button : buttons)
            addAndMakeVisible(button);
		for (auto& modDial : modDials)
			addAndMakeVisible(modDial);
        modDials[0].setVisible(false);

        const auto fontKnobs = font::dosisExtraBold();

		makeKnob(PID::VoiceAttack, get(kKnobs::kEnvAmpAttack));
		makeKnob(PID::VoiceDecay, get(kKnobs::kEnvAmpDecay));
		makeKnob(PID::VoiceSustain, get(kKnobs::kEnvAmpSustain));
		makeKnob(PID::VoiceRelease, get(kKnobs::kEnvAmpRelease));
		getModDial(kKnobs::kEnvAmpAttack).attach(PID::VoiceAttack);
		getModDial(kKnobs::kEnvAmpDecay).attach(PID::VoiceDecay);
		getModDial(kKnobs::kEnvAmpSustain).attach(PID::VoiceSustain);
		getModDial(kKnobs::kEnvAmpRelease).attach(PID::VoiceRelease);
		makeTextLabel(get(kLabels::kEnvAmpAtk), "A", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(get(kLabels::kEnvAmpDcy), "D", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(get(kLabels::kEnvAmpSus), "S", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(get(kLabels::kEnvAmpRls), "R", fontKnobs, Just::centred, CID::Txt);

        auto& titleMacro = get(kLabels::kTitleMacro);
        makeTextLabel(titleMacro, "// Macro //", titleFont, Just::centred, CID::Txt);
        makeKnob(PID::Macro, get(kKnobs::kMacro), false);
		
        auto& buttonMacroRel = get(kButtons::kMacroRel);
		makeTextButton(buttonMacroRel, "Rel", "Switch between absolute and relative macro modulation.", CID::Interact);
		buttonMacroRel.onPaint = makeButtonOnPaint(Button::Type::kToggle);
        buttonMacroRel.value = utils.audioProcessor.params.isModDepthAbsolute() ? 0.f : 1.f;
        buttonMacroRel.onClick = [&u = utils, &b = buttonMacroRel](const Mouse&)
        {
            u.audioProcessor.params.switchModDepthAbsolute();
            b.value = u.audioProcessor.params.isModDepthAbsolute() ? 0.f : 1.f;
        };

		auto& buttonMacroSwap = get(kButtons::kMacroSwap);
		makeTextButton(buttonMacroSwap, "Swap", "Swap all parameters' main values with their modulation destinations", CID::Interact);
        buttonMacroSwap.onPaint = makeButtonOnPaint(Button::Type::kTrigger);
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

        for(auto& materialView: materialViews)
			addChildComponent(materialView);
        materialViews[0].setVisible(true);

        {
            String ab[] = { "A", "B" };
			String tooltips[] =
            {
                "Observe material a.",
                "Observe material b."
            };
            for (auto i = 0; i < 2; ++i)
            {
                const auto& btnName = ab[i];
                const auto btnIdx = static_cast<int>(kButtons::kMaterialA) + i;
                auto& btn = get(static_cast<kButtons>(btnIdx));
                makeTextButton(btn, btnName, tooltips[i], CID::Interact);
                btn.onPaint = makeButtonOnPaint(Button::Type::kToggle);
                btn.onClick = [&, i](const Mouse&)
                {
                    for (auto& mv : materialViews)
                        mv.setVisible(false);
                    materialViews[i].setVisible(true);
					
					for (auto j = 0; j < materialViews.size(); ++j)
                    {
                        if (i != j)
                        {
                            auto buttonsIdx = static_cast<int>(kButtons::kMaterialA) + j;
                            auto& btn = buttons[buttonsIdx];
                            btn.value = 0.f;
                            btn.repaint();
                        }
                    }
                    buttons[static_cast<int>(kButtons::kMaterialA) + i].value = 1.f;
                };
            }
			get(kButtons::kMaterialA).value = 1.f;
        }

		addChildComponent(materialDropDown);
        materialDropDown.add
        ([](Graphics& g, const Button& btn)
        {
            setCol(g, CID::Interact);
			g.drawFittedText("Rescue Overlaps", btn.getLocalBounds(), Just::centred, 1);
        },[&modalFilter = utils.audioProcessor.pluginProcessor.modalFilter, &btns = buttons](const Mouse&)
        {
		    auto btnsIdx = static_cast<int>(kButtons::kMaterialA);
            auto matIdx = 0;
            for (auto i = 1; i < 2; ++i)
				if (btns[btnsIdx + i].value == 1.f)
				{
					matIdx = i;
					break;
				}

            auto& material = modalFilter.getMaterial(matIdx);
            auto& peakInfos = material.peakInfos;
            const auto numFilters = dsp::modal2::NumFilters;
            while (true)
            {
                Point duplicate;
			    bool duplicateFound = false;
			    for (auto i = 0; i < numFilters; ++i)
			    {
				    auto r0 = peakInfos[i].ratio;
				    for (auto j = i + 1; j < numFilters; ++j)
				    {
                        if (i != j)
                        {
						    const auto r1 = peakInfos[j].ratio;
                            if (r0 == r1)
                            {
							    duplicate = { i, j };
                                duplicateFound = true;
                                break;
                            }
                        }
					   
				    }
			    }
			    if (!duplicateFound)
				    return;

                const auto dIdx1 = duplicate.y;
			    const auto d1Mag = peakInfos[dIdx1].mag;

                for (auto i = dIdx1; i < numFilters - 1; ++i)
                {
				    peakInfos[i].mag = peakInfos[i + 1].mag;
				    peakInfos[i].ratio = peakInfos[i + 1].ratio;
                }

			    peakInfos[numFilters - 1].mag = d1Mag;

                const auto maxRatio = peakInfos[numFilters - 1].ratio;
			    peakInfos[numFilters - 1].ratio = maxRatio + 1.f;

                material.status = dsp::modal2::Status::UpdatedMaterial;
            }
        });

        materialDropDown.init();

		auto& materialDropDownButton = get(kButtons::kMaterialDropDown);
        makeTextButton(materialDropDownButton, "V", "Here you can find additional modal material features.", CID::Interact);
        materialDropDownButton.onPaint = makeButtonOnPaint(Button::Type::kTrigger);
        materialDropDownButton.onClick = [&m = materialDropDown](const Mouse&)
		{
			m.setVisible(!m.isVisible());
		};

        const auto justModalSliders = Just::bottomRight;

        auto& modalBlendLabel = get(kLabels::kModalBlend);
		makeTextLabel(modalBlendLabel, "Blend", fontKnobs, justModalSliders, CID::Txt);
		auto& modalBlendKnob = get(kKnobs::kModalBlend);
		makeSlider(PID::ModalBlend, modalBlendKnob);
		auto& blendModDial = getModDial(kKnobs::kModalBlend);
        blendModDial.attach(PID::ModalBlend);
        blendModDial.verticalDrag = false;
		auto& modalBlendEnvKnob = get(kKnobs::kModalBlendEnv);
		makeSlider(PID::ModalBlendEnv, modalBlendEnvKnob);
		auto& modBlendEnvModDial = getModDial(kKnobs::kModalBlendEnv);
		modBlendEnvModDial.attach(PID::ModalBlendEnv);
		modBlendEnvModDial.verticalDrag = false;

		auto& modalSpreizungLabel = get(kLabels::kModalSpreizung);
		makeTextLabel(modalSpreizungLabel, "Spreizung", fontKnobs, justModalSliders, CID::Txt);
		auto& modalSpreizungKnob = get(kKnobs::kModalSpreizung);
		makeSlider(PID::ModalSpreizung, modalSpreizungKnob);
		auto& spreizungModDial = getModDial(kKnobs::kModalSpreizung);
		spreizungModDial.attach(PID::ModalSpreizung);
		spreizungModDial.verticalDrag = false;
		auto& modalSpreizungEnvKnob = get(kKnobs::kModalSpreizungEnv);
		makeSlider(PID::ModalSpreizungEnv, modalSpreizungEnvKnob);
		auto& spreizungEnvModDial = getModDial(kKnobs::kModalSpreizungEnv);
		spreizungEnvModDial.attach(PID::ModalSpreizungEnv);
		spreizungEnvModDial.verticalDrag = false;

		auto& modalHarmonieLabel = get(kLabels::kModalHarmonie);
		makeTextLabel(modalHarmonieLabel, "Harmonie", fontKnobs, justModalSliders, CID::Txt);
		auto& modalHarmonieKnob = get(kKnobs::kModalHarmonie);
		makeSlider(PID::ModalHarmonie, modalHarmonieKnob);
		auto& harmonieModDial = getModDial(kKnobs::kModalHarmonie);
		harmonieModDial.attach(PID::ModalHarmonie);
		harmonieModDial.verticalDrag = false;
		auto& modalHarmonieEnvKnob = get(kKnobs::kModalHarmonieEnv);
		makeSlider(PID::ModalHarmonieEnv, modalHarmonieEnvKnob);
		auto& harmonieEnvModDial = getModDial(kKnobs::kModalHarmonieEnv);
		harmonieEnvModDial.attach(PID::ModalHarmonieEnv);
		harmonieEnvModDial.verticalDrag = false;

		auto& modalKraftLabel = get(kLabels::kModalKraft);
		makeTextLabel(modalKraftLabel, "Kraft", fontKnobs, justModalSliders, CID::Txt);
		auto& modalKraftKnob = get(kKnobs::kModalKraft);
		makeSlider(PID::ModalKraft, modalKraftKnob);
		auto& kraftModDial = getModDial(kKnobs::kModalKraft);
		kraftModDial.attach(PID::ModalKraft);
		kraftModDial.verticalDrag = false;
		auto& modalKraftEnvKnob = get(kKnobs::kModalKraftEnv);
		makeSlider(PID::ModalKraftEnv, modalKraftEnvKnob);
		auto& kraftEnvModDial = getModDial(kKnobs::kModalKraftEnv);
		kraftEnvModDial.attach(PID::ModalKraftEnv);
		kraftEnvModDial.verticalDrag = false;

		auto& modalResoLabel = get(kLabels::kModalReso);
		makeTextLabel(modalResoLabel, "Resonanz", fontKnobs, justModalSliders, CID::Txt);
		auto& modalResoKnob = get(kKnobs::kModalReso);
		makeSlider(PID::ModalResonanz, modalResoKnob);
		auto& resoModDial = getModDial(kKnobs::kModalReso);
		resoModDial.attach(PID::ModalResonanz);
		resoModDial.verticalDrag = false;

        const auto& user = *audioProcessor.state.props.getUserSettings();
        addChildComponent(toast);
		
        const auto editorWidth = user.getDoubleValue("EditorWidth", EditorWidth);
        const auto editorHeight = user.getDoubleValue("EditorHeight", EditorHeight);
        setOpaque(true);
        setResizable(true, true);
        setSize
        (
            static_cast<int>(editorWidth),
            static_cast<int>(editorHeight)
        );

        utils.audioProcessor.pluginProcessor.editorExists.store(true);
    }

    Editor::~Editor()
    {
        utils.audioProcessor.pluginProcessor.editorExists.store(false);
    }
    
    void Editor::paint(Graphics& g)
    {
        const auto bgCol = getColour(CID::Bg);
        const auto modulatorAreaCol = getColour(CID::Txt);
        const auto titleAreaCol = getColour(CID::Mod);
        g.fillAll(bgCol);
        
        const auto bounds = getLocalBounds().toFloat();
		Stroke stroke(utils.thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
        Path p;

        // line between the 2 resynthesis modules
        {
            p.startNewSubPath(layout(17, 2));
            p.lineTo(layout(17, -1));
        }

        // line between the main section and the tree animation
        {
            p.startNewSubPath(layout(-6, -10));
            p.lineTo(layout(-6, -2));
            g.strokePath(p, stroke);
            p.clear();
        }

        // sorrounds modulator area
        {
            const auto startPos = layout(8, 0);
            p.startNewSubPath(startPos);
            p.lineTo(layout(8, 2));
            p.lineTo(layout(8, 8));
            p.quadraticTo(layout(8, 9), layout(7, 9));
            p.quadraticTo(layout(8, 9), layout(8, 10));
            p.lineTo(layout(8, 16));
            p.quadraticTo(layout(8, -2), layout(9, -2));
            p.lineTo(layout(-1, -2));
            closePathOverBounds(p, bounds, startPos, utils.thicc, 1, 3, 0, 2);

            g.setColour(modulatorAreaCol.darker(4.f).withMultipliedSaturation(.25f));
            g.fillPath(p);
            g.setColour(modulatorAreaCol);
            g.strokePath(p, stroke);
            p.clear();
        }

        // draw bg of material view
        {
            const auto modalMaterialViewArea = materialViews[0].getBounds().toFloat();
            g.setColour(Colour(0x44000000));
			g.fillRoundedRectangle(modalMaterialViewArea.expanded(utils.thicc), utils.thicc);
        }

        // sorrounds title and i/o section
        {
            auto startPos = layout(0, 0);
            p.startNewSubPath(startPos);
            p.quadraticTo(layout(4, 0), layout(4, 2));
            p.lineTo(layout(23, 2));
            p.quadraticTo(layout(25, 2), layout(25, 4));
            p.lineTo(layout(25, 9));
            p.quadraticTo(layout(31, 9), layout(31, 17));
            closePathOverBounds(p, bounds, startPos, utils.thicc, 1, 2, 0, 0);

            g.setColour(titleAreaCol.darker(4.f).withMultipliedSaturation(.2f));
            g.fillPath(p);
            g.setColour(titleAreaCol);
            g.strokePath(p, stroke);
            p.clear();
        }

        // arc above the tree animation
        {
            p.startNewSubPath(layout(25, 8));
            p.quadraticTo(layout(31, 8), layout(31, 17));
        }

        // arc that seperates title from i/o section
        {
            p.startNewSubPath(layout(23, 2));
            p.quadraticTo(layout(23, 0), layout(25, 0));
        }

        g.strokePath(p, stroke);
    }

    void Editor::paintOverChildren(Graphics& g)
    {
       layout.paint(g, Colour(0x05ffffff));
    }

    void Editor::resized()
    {
        if (getWidth() < EditorMinWidth)
            return setSize(EditorMinWidth, getHeight());
        if (getHeight() < EditorMinHeight)
            return setSize(getWidth(), EditorMinHeight);

        utils.resized();
        layout.resized(getLocalBounds());

        layout.place(get(kLabels::kTitle), 4, 0.f, 8, 1.3f);
        get(kLabels::kTitle).setMaxHeight();
		layout.place(get(kLabels::kDev), 4, 1.2f, 8, .7f);
        get(kLabels::kDev).setMaxHeight();

        auto& titleMacro = get(kLabels::kTitleMacro);
        layout.place(titleMacro, -5, 0, 4, 1);
        titleMacro.setMaxHeight();
        layout.place(get(kKnobs::kMacro), -5, 1, 2, 2, true);
		layout.place(get(kButtons::kMacroRel), -3, 1, 2, 1);
		layout.place(get(kButtons::kMacroSwap), -3, 2, 2, 1);
        
		layout.place(get(kKnobs::kEnvAmpAttack), 0, 6, 2, 2, true);
		layout.place(get(kKnobs::kEnvAmpDecay), 2, 6, 2, 2, true);
		layout.place(get(kKnobs::kEnvAmpSustain), 4, 6, 2, 2, true);
		layout.place(get(kKnobs::kEnvAmpRelease), 6, 6, 2, 2, true);
		layout.place(get(kLabels::kEnvAmpAtk), 0, 8, 2, 1);
		layout.place(get(kLabels::kEnvAmpDcy), 2, 8, 2, 1);
		layout.place(get(kLabels::kEnvAmpSus), 4, 8, 2, 1);
		layout.place(get(kLabels::kEnvAmpRls), 6, 8, 2, 1);

        layout.place(get(kLabels::kTitleModal), 9, 2, 7, 1);
        layout.place(get(kLabels::kTitleFlanger), 18, 2, 7, 1);

        auto modalY = 4;
        auto modalX = 8;

        for (auto i = 0; i < 2; ++i)
        {
            const auto idx = static_cast<int>(kButtons::kMaterialA) + i;
            const auto x = static_cast<float>(modalX + i);
            layout.place(get(static_cast<kButtons>(idx)), x, modalY, 1.f, 1);
        }

		layout.place(get(kButtons::kMaterialDropDown), 15.5f, modalY, 1.f, 1);

        const auto moduleWidth = 9;
        const auto materialHeight = 4;
        ++modalY;

		const auto materialBounds = layout(modalX, modalY, moduleWidth, materialHeight)
            .reduced(utils.thicc * 2.f).toNearestInt();

        for (auto i = 0; i < materialViews.size(); ++i)
            materialViews[i].setBounds(materialBounds);
		materialDropDown.setBounds(materialBounds);

        modalY += materialHeight;

        const auto modalLabelWidth = 2;
		const auto modalKnobX = modalX + modalLabelWidth;
		const auto modalKnobWidth = 3;
		const auto modalEnvX = modalKnobX + modalKnobWidth + 1;
        const auto modalEnvWidth = 2;

        layout.place(get(kLabels::kModalBlend), modalX, modalY, modalLabelWidth, 1);
        layout.place(get(kKnobs::kModalBlend), modalKnobX, modalY, modalKnobWidth, 1);
        layout.place(get(kKnobs::kModalBlendEnv), modalEnvX, modalY, modalEnvWidth, 1);

		++modalY;

		layout.place(get(kLabels::kModalSpreizung), modalX, modalY, modalLabelWidth, 1);
		layout.place(get(kKnobs::kModalSpreizung), modalKnobX, modalY, modalKnobWidth, 1);
		layout.place(get(kKnobs::kModalSpreizungEnv), modalEnvX, modalY, modalEnvWidth, 1);

		++modalY;


		layout.place(get(kLabels::kModalHarmonie), modalX, modalY, modalLabelWidth, 1);
		layout.place(get(kKnobs::kModalHarmonie), modalKnobX, modalY, modalKnobWidth, 1);
		layout.place(get(kKnobs::kModalHarmonieEnv), modalEnvX, modalY, modalEnvWidth, 1);

        ++modalY;

		layout.place(get(kLabels::kModalKraft), modalX, modalY, modalLabelWidth, 1);
		layout.place(get(kKnobs::kModalKraft), modalKnobX, modalY, modalKnobWidth, 1);
		layout.place(get(kKnobs::kModalKraftEnv), modalEnvX, modalY, modalEnvWidth, 1);

        ++modalY;

		layout.place(get(kLabels::kModalReso), modalX, modalY, modalLabelWidth, 1);
		layout.place(get(kKnobs::kModalReso), modalKnobX, modalY, modalKnobWidth, 1);

        ++modalY;

        LabelGroup envAmpLabels;
        envAmpLabels.add(get(kLabels::kEnvAmpAtk));
        envAmpLabels.add(get(kLabels::kEnvAmpDcy));
        envAmpLabels.add(get(kLabels::kEnvAmpSus));
        envAmpLabels.add(get(kLabels::kEnvAmpRls));
        envAmpLabels.setMaxHeight();

        LabelGroup modalKnobLabels;
        modalKnobLabels.add(get(kLabels::kModalBlend));
		modalKnobLabels.add(get(kLabels::kModalSpreizung));
		modalKnobLabels.add(get(kLabels::kModalHarmonie));
		modalKnobLabels.add(get(kLabels::kModalKraft));
		modalKnobLabels.add(get(kLabels::kModalReso));
        modalKnobLabels.setMaxHeight();

        locateAtKnob(getModDial(kKnobs::kEnvAmpAttack), get(kKnobs::kEnvAmpAttack));
        locateAtKnob(getModDial(kKnobs::kEnvAmpDecay), get(kKnobs::kEnvAmpDecay));
        locateAtKnob(getModDial(kKnobs::kEnvAmpSustain), get(kKnobs::kEnvAmpSustain));
        locateAtKnob(getModDial(kKnobs::kEnvAmpRelease), get(kKnobs::kEnvAmpRelease));
        locateAtSlider(getModDial(kKnobs::kModalBlend), get(kKnobs::kModalBlend));
		locateAtSlider(getModDial(kKnobs::kModalSpreizung), get(kKnobs::kModalSpreizung));
		locateAtSlider(getModDial(kKnobs::kModalHarmonie), get(kKnobs::kModalHarmonie));
        locateAtSlider(getModDial(kKnobs::kModalKraft), get(kKnobs::kModalKraft));
		locateAtSlider(getModDial(kKnobs::kModalReso), get(kKnobs::kModalReso));
		locateAtSlider(getModDial(kKnobs::kModalBlendEnv), get(kKnobs::kModalBlendEnv));
		locateAtSlider(getModDial(kKnobs::kModalSpreizungEnv), get(kKnobs::kModalSpreizungEnv));
		locateAtSlider(getModDial(kKnobs::kModalHarmonieEnv), get(kKnobs::kModalHarmonieEnv));
		locateAtSlider(getModDial(kKnobs::kModalKraftEnv), get(kKnobs::kModalKraftEnv));

        layout.place(genAni, -6, 8, 5, 9);
        layout.place(tooltip, 0, -2, 31, 1);

		const auto toastWidth = static_cast<int>(utils.thicc * 28.f);
        const auto toastHeight = toastWidth * 3 / 4;
		toast.setSize(toastWidth, toastHeight);

        {
            LabelGroup titlesGroup;
            titlesGroup.add(get(kLabels::kTitleModal));
            titlesGroup.add(get(kLabels::kTitleFlanger));
            titlesGroup.setMaxHeight();
        }

        auto& user = *audioProcessor.state.props.getUserSettings();
		const auto editorWidth = static_cast<double>(getWidth());
		const auto editorHeight = static_cast<double>(getHeight());
		user.setValue("EditorWidth", editorWidth);
		user.setValue("EditorHeight", editorHeight);
	}

    void Editor::mouseEnter(const Mouse&)
    {
        evtMember(evt::Type::TooltipUpdated);
    }

    void Editor::mouseUp(const Mouse&)
    {
        evtMember(evt::Type::ClickedEmpty);
    }
}