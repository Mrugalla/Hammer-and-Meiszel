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
			Label(utils)
        },
        knobs
        {
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
        modParams
        {
            HnMParam(utils),
			HnMParam(utils),
			HnMParam(utils),
			HnMParam(utils),
			HnMParam(utils)
        },
        materialViews
        {
            ModalMaterialView(utils, utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(0)),
            ModalMaterialView(utils, utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(1))
        },
        materialDropDown(utils)
    {
        layout.initGrid(GridNumX, GridNumY);
        
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

		modParams[0].init(*this, "Blend", PID::ModalBlend, PID::ModalBlendBreite, PID::ModalBlendEnv);
		modParams[1].init(*this, "Spreizung", PID::ModalSpreizung, PID::ModalSpreizungBreite, PID::ModalSpreizungEnv);
		modParams[2].init(*this, "Harmonie", PID::ModalHarmonie, PID::ModalHarmonieBreite, PID::ModalHarmonieEnv);
		modParams[3].init(*this, "Kraft", PID::ModalKraft, PID::ModalKraftBreite, PID::ModalKraftEnv);
		modParams[4].init(*this, "Resonanz", PID::ModalResonanz, PID::ModalResonanzBreite, PID::ModalResonanzEnv);

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
        const auto thicc = utils.thicc;
		Stroke stroke(utils.thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
        Path p;

        // title area
        {
            auto startPos = layout(0, TitleHeight);
            p.startNewSubPath(startPos);
            p.lineTo(layout(-1, TitleHeight));
            closePathOverBounds(p, bounds, startPos, thicc, 1, 2, 0, 0);
            g.setColour(titleAreaCol);
            g.strokePath(p, stroke);

            static constexpr auto ParamsX = -13;
            static constexpr float ContentHeight = static_cast<float>(TitleHeight) - TitleContentMargin * 2.f;

            g.drawEllipse(maxQuadIn(layout(ParamsX, TitleContentMargin, 1, ContentHeight)), thicc); // < prev preset
            g.drawEllipse(maxQuadIn(layout(ParamsX + 1, TitleContentMargin, 1, ContentHeight)), thicc); // > next preset
            g.drawEllipse(maxQuadIn(layout(ParamsX + 2, TitleContentMargin, 1, ContentHeight)), thicc); // v main menu
            g.drawEllipse(maxQuadIn(layout(ParamsX + 3, TitleContentMargin, 1, ContentHeight)), thicc); // parameter randomizer
            // tuning area
            g.drawRect(layout(ParamsX + 4, TitleContentMargin, 2, ContentHeight), thicc); // xen
            g.drawRect(layout(ParamsX + 6, TitleContentMargin, 2, ContentHeight), thicc); // tune
            g.drawRect(layout(ParamsX + 8, TitleContentMargin, 2, ContentHeight), thicc); // ref pitch
            g.drawRect(layout(ParamsX + 10, TitleContentMargin, 2, ContentHeight), thicc); // pb-range

            // preset area
            p.clear();
            p.startNewSubPath(layout(SidePanelWidth, TitleContentMargin));
            p.lineTo(layout(SidePanelWidth, TitleContentMargin + ContentHeight));
            p.lineTo(layout(ParamsX, TitleContentMargin + ContentHeight));
            p.lineTo(layout(ParamsX, TitleContentMargin));
            p.closeSubPath();
            g.strokePath(p, stroke);
        }

        // side panels
        {
            p.clear();
            auto startPos = layout(0, TitleHeight);
            p.startNewSubPath(startPos);
            p.lineTo(layout(SidePanelWidth, TitleHeight));
            p.lineTo(layout(SidePanelWidth, -1 - TooltipHeight));
            p.lineTo(layout(0, -1 - TooltipHeight));
            closePathOverBounds(p, bounds, startPos, thicc, 0, 2, 2, 2);
            g.strokePath(p, stroke);

            p.clear();
            startPos = layout(-1, TitleHeight);
            p.startNewSubPath(startPos);
            p.lineTo(layout(- 1 - SidePanelWidth, TitleHeight));
            p.lineTo(layout(-1 - SidePanelWidth, -1 - TooltipHeight));
            p.lineTo(layout(-1, -1 - TooltipHeight));
            closePathOverBounds(p, bounds, startPos, thicc, 1, 2, 2, 2);
            g.strokePath(p, stroke);

            g.drawLine(LineF(layout(0, SidePanelMidY), layout(SidePanelWidth, SidePanelMidY)), thicc);
            g.drawLine(LineF(layout(-1 - SidePanelWidth, SidePanelMidY), layout(-1, SidePanelMidY)), thicc);
        }

        // fx chain area
        {
            p.clear();
            p.startNewSubPath(layout(FXChainX, FXChainY));
            p.lineTo(layout(FXChainX, FXChainBottom));
            p.lineTo(layout(FXChainRight, FXChainBottom));
            p.lineTo(layout(FXChainRight, FXChainY));
            p.closeSubPath();
            g.strokePath(p, stroke);
        }

        // module area
        {
            // modal
            {
                g.drawRect(layout(ModParamsTableTopX, ModParamsTableTopY, ModParamsTableTopWidth, ModParamsTableTopHeight));
            }
        }

        // tooltip area
        g.drawLine(layout.getLine(0, TooltipY, -1, TooltipY), thicc);

        // draw bg of material view
        {
            const auto modalMaterialViewArea = materialViews[0].getBounds().toFloat();
            setCol(g, CID::Darken);
            g.fillRoundedRectangle(modalMaterialViewArea.expanded(utils.thicc), utils.thicc);
        }
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
        
        const auto titleHeightHalf = static_cast<float>(TitleHeight) * .5f;

        layout.place(get(kLabels::kTitle), 0.f, 0.f, SidePanelWidth, titleHeightHalf);
        get(kLabels::kTitle).setMaxHeight();
        layout.place(get(kLabels::kDev), 0.f, titleHeightHalf, SidePanelWidth, titleHeightHalf);
        get(kLabels::kDev).setMaxHeight();
        
        auto& titleModal = get(kLabels::kTitleModal);
        layout.place(titleModal, ModuleTitleX, ModuleTitleY, ModuleTitleWidth, ModuleTitleHeight);
        titleModal.setMaxHeight();
        //auto& titleComb = get(kLabels::kTitleFlanger);
        //remove commenting out when tabbed window implemented
        //layout.place(titleComb, ModuleTitleX, ModuleTitleY, ModuleTitleWidth, ModuleTitleHeight);
        //titleComb.setMaxHeight();

        const auto materialBounds = layout(ModMatEditorX, ModMatEditorY, ModMatEditorWidth, ModMatEditorHeight)
            .reduced(utils.thicc * 2.f).toNearestInt();
        for (auto i = 0; i < materialViews.size(); ++i)
            materialViews[i].setBounds(materialBounds);

		const auto modalParametersBounds = layout(ModParamsX, ModParamsY, ModParamsWidth, ModParamsHeight)
			.reduced(utils.thicc * 2.f);
        {
            const auto x = modalParametersBounds.getX();
			const auto w = modalParametersBounds.getWidth();
			const auto numParamsInv = 1.f / static_cast<float>(modParams.size());
            const auto h = modalParametersBounds.getHeight() * numParamsInv;

            auto y = modalParametersBounds.getY();
            for (auto i = 0; i < modParams.size(); ++i)
            {
                auto modalParamBounds = BoundsF(x, y, w, h).reduced(utils.thicc);
                modParams[i].setBounds(modalParamBounds);
				y += h;
            }
        }
        LabelGroup modalKnobLabels;
        for(auto& m: modParams)
            modalKnobLabels.add(m.label);
        modalKnobLabels.setMaxHeight();

        layout.place(genAni, GenAniX, GenAniY, GenAniWidth, GenAniHeight);
        layout.place(tooltip, 0, TooltipY, GridNumX, TooltipHeight);

        const auto toastWidth = static_cast<int>(utils.thicc * 28.f);
        const auto toastHeight = toastWidth * 3 / 4;
        toast.setSize(toastWidth, toastHeight);

        /*
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

		materialDropDown.setBounds(materialBounds);
        */

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