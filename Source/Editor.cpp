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

    Button& Editor::get(kButtons i) noexcept
    {
		return buttons[static_cast<int>(i)];
    }

    void loadBounds(Editor& e)
    {
        const auto& user = *e.audioProcessor.state.props.getUserSettings();
        const auto editorWidth = user.getIntValue("EditorWidth", EditorWidth);
        const auto editorHeight = user.getIntValue("EditorHeight", EditorHeight);
        e.setOpaque(true);
        e.setResizable(true, true);
        e.setSize(editorWidth, editorHeight);
    }

    Editor::Editor(Processor& p) :
        AudioProcessorEditor(&p),
        audioProcessor(p),
        utils(*this, p),
        layout(),
        evtMember(utils.eventSystem, makeEvt(*this)),
        tooltip(utils),
        genAni(utils),
        voiceGrid(utils),
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
			Label(utils)
        },
        envGens
        {
			EnvelopeGeneratorMultiVoiceEditor
            (
                utils,
                "Gain Envelope:",
                PID::EnvGenAmpAttack,
		        PID::EnvGenAmpDecay,
		        PID::EnvGenAmpSustain,
		        PID::EnvGenAmpRelease
			),
			EnvelopeGeneratorMultiVoiceEditor
            (
                utils,
                "Mod Envelope:",
				PID::EnvGenModAttack,
				PID::EnvGenModDecay,
				PID::EnvGenModSustain,
				PID::EnvGenModRelease
			)
        },
        macro(utils),
        buttons
        {
            Button(utils),
			Button(utils),
            Button(utils),
            Button(utils),
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
		ioSliders
	    {
			SidePanelParam(utils),
            SidePanelParam(utils),
            SidePanelParam(utils)
	    },
        materialViews
        {
            ModalMaterialView
            (
                utils,
                utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(0),
				utils.audioProcessor.pluginProcessor.modalFilter.getActives()
            ),
            ModalMaterialView
            (
                utils,
                utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(1),
                utils.audioProcessor.pluginProcessor.modalFilter.getActives()
            )
        },
        materialDropDown(utils)
    {
        layout.initGrid(GridNumX, GridNumY);
        
        addAndMakeVisible(genAni);
		addAndMakeVisible(voiceGrid);
		voiceGrid.init([&](VoiceGrid<dsp::AutoMPE::VoicesSize>::Voices& voices)
		{
            bool updated = false;
            const auto& pp = audioProcessor.pluginProcessor.parallelProcessor;
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
        addAndMakeVisible(tooltip);

        const auto fontKnobs = font::dosisExtraBold();
        String dev(JucePlugin_Manufacturer);
        auto titleFont = font::flx();
        const auto devFont = titleFont;
        titleFont.setExtraKerningFactor(.2f);
        makeTextLabel(get(kLabels::kDev), "crafted by Florian " + dev, devFont, Just::centred, CID::Txt);
        const auto moduleTitleFont = font::dosisBold();
		makeTextLabel(get(kLabels::kTitleModal), "// Modal \\\\", moduleTitleFont, Just::centred, CID::Txt);
		makeTextLabel(get(kLabels::kTitleFlanger), "// Flanger \\\\", moduleTitleFont, Just::centred, CID::Txt);

        auto sz = String(juce::CharPointer_UTF8("\xc3\x9f"));
        String name(JucePlugin_Name);
        for (auto i = 0; i < name.length(); ++i)
            if (name[i] == 's' && name[i + 1] == 'z')
                name = name.replaceSection(i, 2, sz);
        
        makeTextLabel(get(kLabels::kTitle), name, titleFont, Just::centred, CID::Txt);
        
        for (auto& label : labels)
            addAndMakeVisible(label);
        addAndMakeVisible(macro);
        for (auto& button : buttons)
            addAndMakeVisible(button);
		
        for (auto& envGen : envGens)
            addAndMakeVisible(envGen);

        auto& titleMacro = get(kLabels::kTitleMacro);
        makeTextLabel(titleMacro, "Macro", fontKnobs, Just::centred, CID::Txt);
        makeKnob(PID::Macro, macro, false);
		
        {
            auto& buttonMacroRel = get(kButtons::kMacroRel);
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
            makeTextButton(buttonMacroRel, textAbsRel, "Switch between absolute and relative macro modulation.", CID::Interact);
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
        
        {
            auto& buttonMacroSwap = get(kButtons::kMacroSwap);
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
				btn.type = Button::Type::kToggle;
                btn.onPaint = makeButtonOnPaint(true);
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

        {
			auto& buttonSolo = get(kButtons::kMaterialSolo);
            addAndMakeVisible(buttonSolo);
            makeTextButton(buttonSolo, "S", "Solo the selected partials with this button!", CID::Interact);
            buttonSolo.value = 0.f;
            buttonSolo.type = Button::Type::kToggle;
            buttonSolo.onClick = [&](const Mouse&)
            {
                buttonSolo.value = std::round(1.f - buttonSolo.value);
				utils.audioProcessor.pluginProcessor.modalFilter.setSolo(buttonSolo.value > .5f);
                for (auto& mv : materialViews)
                {
                    if (mv.isShowing())
                    {
                        mv.updateActives(buttonSolo.value > .5f);
						mv.repaint();
                    }
                }
            };
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
        materialDropDownButton.onPaint = makeButtonOnPaint(true);
        materialDropDownButton.onClick = [&m = materialDropDown](const Mouse&)
		{
			m.setVisible(!m.isVisible());
		};

		modParams[0].init(*this, "Blend", PID::ModalBlend, PID::ModalBlendBreite, PID::ModalBlendEnv);
		modParams[1].init(*this, "Spreizung", PID::ModalSpreizung, PID::ModalSpreizungBreite, PID::ModalSpreizungEnv);
		modParams[2].init(*this, "Harmonie", PID::ModalHarmonie, PID::ModalHarmonieBreite, PID::ModalHarmonieEnv);
		modParams[3].init(*this, "Kraft", PID::ModalKraft, PID::ModalKraftBreite, PID::ModalKraftEnv);
		modParams[4].init(*this, "Resonanz", PID::ModalResonanz, PID::ModalResonanzBreite, PID::ModalResonanzEnv);

        ioSliders[0].init(*this, "Wet", PID::GainWet);
		ioSliders[1].init(*this, "Mix", PID::Mix);
		ioSliders[2].init(*this, "Out", PID::GainOut);
        {
            auto& powerButton = get(kButtons::kPower);
            makeParameter(powerButton, PID::Power, Button::Type::kToggle, makeButtonOnPaintPower());
            const auto powerButtonOnClick = powerButton.onClick;
            powerButton.onClick = [&, oc = powerButtonOnClick](const Mouse& mouse)
            {
                oc(mouse);
                repaint();
            };
        }
        {
			auto& deltaButton = get(kButtons::kDelta);
			makeParameter(deltaButton, PID::Delta, Button::Type::kToggle, makeButtonOnPaintPolarity());
        }
        {
			auto& midSideButton = get(kButtons::kMidSide);
			makeParameter(midSideButton, PID::StereoConfig, Button::Type::kChoice, "L/R;M/S");
		}

        addChildComponent(toast);
        loadBounds(*this);
        utils.audioProcessor.pluginProcessor.editorExists.store(true);
    }

    Editor::~Editor()
    {
        utils.audioProcessor.pluginProcessor.editorExists.store(false);
    }
    
    void Editor::paint(Graphics& g)
    {
        const auto bgCol = getColour(CID::Bg);
        const auto titleAreaCol = getColour(CID::Interact).darker(1.f);
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
            // left panel
            {
                p.clear();
                const auto startPos = layout(0, TitleHeight);
                p.startNewSubPath(startPos);
                p.lineTo(layout(SidePanelWidth, TitleHeight));
                p.lineTo(layout(SidePanelWidth, -1 - TooltipHeight));
                p.lineTo(layout(0, -1 - TooltipHeight));
                closePathOverBounds(p, bounds, startPos, thicc, 0, 2, 2, 2);
                //g.strokePath(p, stroke);
            }
            
            // right panel
            {
                p.clear();
                const auto startPos = layout(-1, GenAniY);
                p.startNewSubPath(startPos);
                p.lineTo(layout(SidePanelRightX, GenAniY));
                p.lineTo(layout(SidePanelRightX, TooltipY));
                p.lineTo(layout(-1, TooltipY));
                closePathOverBounds(p, bounds, startPos, thicc, 1, 2, 2, 2);
                g.setColour(titleAreaCol);
                //g.strokePath(p, stroke);
            }
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
        //g.drawLine(layout.getLine(0, TooltipY, -1, TooltipY), thicc);

        // draw bg of material view
        {
            const auto modalMaterialViewArea = materialViews[0].getBounds().toFloat();
            setCol(g, CID::Darken);
            g.fillRoundedRectangle(modalMaterialViewArea.expanded(utils.thicc), utils.thicc);
        }
    }

    void Editor::paintOverChildren(Graphics& g)
    {
       //layout.paint(g, Colour(0x11ffffff));
	   const auto power = utils.audioProcessor.params(PID::Power).getValue() > .5f;
       if (!power)
           g.fillAll(Colour(0x44000000));
    }

    bool needsResize(Editor& e)
    {
        bool needResize = false;
        const auto w = e.getWidth();
        const auto h = e.getHeight();
        if (w < EditorMinWidth)
        {
            e.setSize(EditorMinWidth, h);
            needResize = true;
        }
        if (h < EditorMinHeight)
        {
            e.setSize(w, EditorMinHeight);
            needResize = true;
        }
		return needResize;
    }
    
    void saveBounds(Editor& e)
    {
        auto& user = *e.audioProcessor.state.props.getUserSettings();
        const auto editorWidth = e.getWidth();
        const auto editorHeight = e.getHeight();
        user.setValue("EditorWidth", editorWidth);
        user.setValue("EditorHeight", editorHeight);
    }

    void resizeLeftPanel(Editor& e, float envGenMargin)
    {
        const auto leftArea = e.layout(0.f, Editor::SidePanelY, Editor::SidePanelWidth, Editor::SidePanelHeight);
		const auto x = leftArea.getX();
		auto y = leftArea.getY();
		const auto w = leftArea.getWidth();
		const auto h = leftArea.getHeight();
		const auto hHalf = h * .5f;
        for (auto i = 0; i < e.envGens.size(); ++i)
		{
			auto& envGen = e.envGens[i];
            envGen.setBounds(BoundsF(x, y, w, hHalf).reduced(envGenMargin).toNearestInt());
			y += hHalf;
		}
    }

    void Editor::resized()
    {
        if (needsResize(*this))
            return;
        saveBounds(*this);

        utils.resized();
        const auto thicc = utils.thicc;
		const auto thicc2 = thicc * 2.f;

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

		resizeLeftPanel(*this, thicc2);
        
        const auto rightArea = layout(SidePanelRightX, SidePanelY, SidePanelWidth, SidePanelHeight);
		
        auto& titleMacro = get(kLabels::kTitleMacro);
        layout.place(titleMacro, SidePanelRightX, SidePanelY + 1.f, SidePanelWidth * .3f , 1.f);
        layout.place(macro, SidePanelRightX + SidePanelWidth * .3f, SidePanelY, SidePanelWidth * .4f, 2.f);
		layout.place(get(kButtons::kMacroRel), SidePanelRightX + SidePanelWidth * .7f, SidePanelY, SidePanelWidth * .3f, 1.f);
		layout.place(get(kButtons::kMacroSwap), SidePanelRightX + SidePanelWidth * .7f, SidePanelY + 1.f, SidePanelWidth * .3f, 1.f);
        titleMacro.setMaxHeight();

        auto& ioOutSlider = ioSliders[static_cast<int>(kIOSliders::kOut)];
        ioOutSlider.setBounds(layout(SidePanelRightX, IOButtonsY - 1.f, SidePanelWidth, 1.f));
		auto& ioMixSlider = ioSliders[static_cast<int>(kIOSliders::kMix)];
		ioMixSlider.setBounds(layout(SidePanelRightX, IOButtonsY  - 2.f, SidePanelWidth, 1.f));
		auto& ioWetSlider = ioSliders[static_cast<int>(kIOSliders::kWet)];
		ioWetSlider.setBounds(layout(SidePanelRightX, IOButtonsY - 3.f, SidePanelWidth, 1.f));
		layout.place(voiceGrid, SidePanelRightX, IOButtonsY - 4.f, SidePanelWidth, 1.f);
        {
            const auto ioButtonWidth = IOButtonsWidth * .333f;
            auto ioButtonX = IOButtonsX;
            layout.place(get(kButtons::kPower), ioButtonX, IOButtonsY, ioButtonWidth, IOButtonsHeight);
			ioButtonX += ioButtonWidth;
			layout.place(get(kButtons::kDelta), ioButtonX, IOButtonsY, ioButtonWidth, IOButtonsHeight);
			ioButtonX += ioButtonWidth;
			layout.place(get(kButtons::kMidSide), ioButtonX, IOButtonsY, ioButtonWidth, IOButtonsHeight);
        }

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
        const auto modMatButtonWidth = ModMatABWidth * .333333f;
        {
			auto x = ModMatABX;
            auto& modMatAButton = get(kButtons::kMaterialA);
            layout.place(modMatAButton, x, ModMatABY, modMatButtonWidth, ModMatABHeight);
			x += modMatButtonWidth;
            auto& modMatBButton = get(kButtons::kMaterialB);
            layout.place(modMatBButton, x, ModMatABY, modMatButtonWidth, ModMatABHeight);
			x += modMatButtonWidth;
            auto& modMatSoloButton = get(kButtons::kMaterialSolo);
            layout.place(modMatSoloButton, x, ModMatABY, modMatButtonWidth, ModMatABHeight);
        }
		
        LabelGroup modalKnobLabels;
        for(auto& m: modParams)
            modalKnobLabels.add(m.label);
        modalKnobLabels.setMaxHeight();

        auto genAniBounds = layout(GenAniX, GenAniY, GenAniWidth, GenAniHeight).reduced(utils.thicc);
		genAni.setBounds(genAniBounds.toNearestInt());
        layout.place(tooltip, 0, TooltipY, GridNumX, TooltipHeight);

        const auto toastWidth = static_cast<int>(utils.thicc * 28.f);
        const auto toastHeight = toastWidth * 3 / 4;
        toast.setSize(toastWidth, toastHeight);

        /*
		layout.place(get(kButtons::kMaterialDropDown), 15.5f, modalY, 1.f, 1);
		materialDropDown.setBounds(materialBounds);
        */
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