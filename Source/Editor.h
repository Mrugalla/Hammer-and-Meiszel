#pragma once
#include "gui/GenAniComp.h"
#include "gui/ModalParamsEditor.h"
#include "gui/IOEditor.h"
#include "gui/Tooltip.h"
#include "gui/DropDownMenu.h"
#include "gui/EnvelopeGeneratorEditor.h"
#include "gui/ModalFilterMaterialView.h"

namespace gui
{
	using AudioProcessorEditor = juce::AudioProcessorEditor;

    struct HnMParam
    {
        HnMParam(Utils& u) :
            layout(),
			label(u),
            param(u),
            breite(u),
            env(u),
            paramMod(u),
            breiteMod(u),
            envMod(u)
        {
            layout.init
            (
                { 1, 3, 3, 3 },
                { 1 }
            );
        }

        void init(AudioProcessorEditor& editor, const String& name, PID pID, PID pIDBreite, PID pIDEnv)
        {
            editor.addAndMakeVisible(label);
            editor.addAndMakeVisible(param);
			editor.addAndMakeVisible(breite);
            editor.addAndMakeVisible(env);
            editor.addAndMakeVisible(paramMod);
			editor.addAndMakeVisible(breiteMod);
            editor.addAndMakeVisible(envMod);

            const auto fontKnobs = font::dosisExtraBold();
            const auto just = Just::bottomRight;

            makeTextLabel(label, name, fontKnobs, just, CID::Txt);
            makeSlider(pID, param);
            makeSlider(pIDBreite, breite);
            makeSlider(pIDEnv, env);
            paramMod.attach(pID);
			breiteMod.attach(pIDBreite);
            envMod.attach(pIDEnv);
            paramMod.verticalDrag = false;
			breiteMod.verticalDrag = false;
            envMod.verticalDrag = false;
        }

        void setBounds(BoundsF bounds)
        {
            layout.resized(bounds);

            layout.place(label, 0, 0, 1, 1);
			layout.place(param, 1.f, 0, .8f, 1);
			layout.place(paramMod, 1.8f, 0, .2f, 1);
			layout.place(breite, 2.f, 0, .8f, 1);
			layout.place(breiteMod, 2.8f, 0, .2f, 1);
			layout.place(env, 3.f, 0, .8f, 1);
			layout.place(envMod, 3.8f, 0, .2f, 1);
        }

        Layout layout;
        Label label;
        Knob param, breite, env;
        ModDial paramMod, breiteMod, envMod;
    };

    struct Editor :
        public AudioProcessorEditor
    {
        static constexpr int GridNumX = 25, GridNumY = 18;
        static constexpr float TitleHeight = 2.f;
        static constexpr float TitleContentMargin = .4f;
        static constexpr float TooltipHeight = 1.f;
        static constexpr float SidePanelWidth = 5.5f;
        static constexpr float FXChainHeight = 1.f;
        static constexpr float ModuleTitleHeight = 2.f;
        static constexpr float ModMatEditorMargin = .5f;
        static constexpr float ModMatABWidth = 2.f;
        static constexpr float ModMatABHeight = 1.f;
        static constexpr float ModParamsTableTopHeight = 1.f;

        static constexpr float GridNumXF = static_cast<float>(GridNumX);
        static constexpr float GridNumYF = static_cast<float>(GridNumY);
		static constexpr float SidePanelY = TitleHeight;
        static constexpr float SidePanelHeight = (GridNumYF - TooltipHeight - SidePanelY);
        static constexpr float SidePanelMidY = SidePanelY + SidePanelHeight * .5f;
        static constexpr float FXChainX = SidePanelWidth;
        static constexpr float FXChainY = SidePanelY;
        static constexpr float FXChainWidth = GridNumXF - SidePanelWidth * 2.f;
        static constexpr float FXChainBottom = FXChainY + FXChainHeight;
        static constexpr float FXChainRight = FXChainX + FXChainWidth;
		static constexpr float SidePanelRightX = FXChainRight;

        static constexpr float IOButtonsHeight = 1.f;
        static constexpr float IOButtonsY = SidePanelMidY - IOButtonsHeight;
		static constexpr float IOButtonsWidth = SidePanelWidth;
		static constexpr float IOButtonsX = SidePanelRightX;
        static constexpr float IOControlsHeight = 3.f;
		static constexpr float IOControlsY = IOButtonsY - IOControlsHeight;

        static constexpr float TooltipY = -1 - TooltipHeight;

        static constexpr float ModuleX = FXChainX;
        static constexpr float ModuleWidth = FXChainWidth;
        static constexpr float ModuleHeight = SidePanelHeight - FXChainHeight;

        static constexpr float ModuleTitleX = ModuleX;
        static constexpr float ModuleTitleY = FXChainBottom;
        static constexpr float ModuleTitleWidth = ModuleWidth;
        static constexpr float ModuleTitleBottom = ModuleTitleY + ModuleTitleHeight;

        static constexpr float ModMatABX = ModuleX + ModMatEditorMargin;
        static constexpr float ModMatABY = ModuleTitleBottom;

        static constexpr float ModMatEditorX = ModMatABX;
        static constexpr float ModMatEditorY = ModMatABY + ModMatABHeight;
        static constexpr float ModMatEditorWidth = ModuleWidth - ModMatEditorMargin * 2.f;
        static constexpr float ModMatEditorHeight = ModuleHeight - ModuleTitleHeight - 5.f - ModMatABHeight - ModParamsTableTopHeight;
        static constexpr float ModMatEditorBottom = ModMatEditorY + ModMatEditorHeight;

		static constexpr float ModParamsX = ModMatEditorX;
		static constexpr float ModParamsY = ModMatEditorBottom;
		static constexpr float ModParamsWidth = ModMatEditorWidth;
		static constexpr float ModParamsBottom = TooltipY;
		static constexpr float ModParamsHeight = ModParamsBottom - ModParamsY;

        static constexpr float GenAniX = -1 - SidePanelWidth;
        static constexpr float GenAniY = SidePanelMidY;
        static constexpr float GenAniWidth = SidePanelWidth;
        static constexpr float GenAniHeight = SidePanelHeight * .5f;

        Editor(Processor&);

        ~Editor() override;

        void paint(Graphics&) override;
        void paintOverChildren(Graphics&) override;
        void resized() override;
        void mouseEnter(const Mouse&) override;
        void mouseUp(const Mouse&) override;

        Processor& audioProcessor;
        Utils utils;
        Layout layout;
        evt::Member evtMember;
        Tooltip tooltip;
        GenAniGrowTrees genAni;
		ModalParamsEditor modParamsEditor;
		IOEditor ioEditor;
        Toast toast;

        enum class kLabels
        {
            kTitle, kDev, kTitleModal, kTitleFlanger,
            kNumLabels
        };
		static constexpr int NumLabels = static_cast<int>(kLabels::kNumLabels);
        std::array<Label, NumLabels> labels;
		enum class kEnvGens { kEnvGenAmp, kEnvGenMod, kNumEnvGens };
		static constexpr int NumEnvGens = static_cast<int>(kEnvGens::kNumEnvGens);
		std::array<EnvelopeGeneratorMultiVoiceEditor, NumEnvGens> envGens;
        enum class kButtons { kMaterialDropDown, kMaterialA, kMaterialB, kMaterialSolo, kNumButtons };
		static constexpr int NumButtons = static_cast<int>(kButtons::kNumButtons);
		std::array<Button, NumButtons> buttons;

        std::array<ModalMaterialView, 2> materialViews;
        DropDownMenu materialDropDown;

        //JUCE_HEAVYWEIGHT_LEAK_DETECTOR(Editor)

        Label& get(kLabels) noexcept;
		Button& get(kButtons) noexcept;
    };
}