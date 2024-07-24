#pragma once
#include "gui/GenAniComp.h"
#include "gui/Tooltip.h"
#include "gui/DropDownMenu.h"
#include "gui/Knob.h"
#include "gui/ModalFilterMaterialView.h"

namespace gui
{
	using AudioProcessorEditor = juce::AudioProcessorEditor;

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
        static constexpr float SidePanelHeight = (GridNumYF - TooltipHeight - TitleHeight);
        static constexpr float SidePanelMidY = TitleHeight + SidePanelHeight * .5f;
        static constexpr float FXChainX = SidePanelWidth;
        static constexpr float FXChainY = TitleHeight;
        static constexpr float FXChainWidth = GridNumXF - SidePanelWidth * 2.f;
        static constexpr float FXChainBottom = FXChainY + FXChainHeight;
        static constexpr float FXChainRight = FXChainX + FXChainWidth;

        static constexpr float ModuleX = FXChainX;
        static constexpr float ModuleWidth = FXChainWidth;
        static constexpr float ModuleHeight = SidePanelHeight - FXChainHeight;
        static constexpr float ModuleWidthHalf = ModuleWidth * .5f;

        static constexpr float ModuleTitleX = ModuleX;
        static constexpr float ModuleTitleY = FXChainBottom;
        static constexpr float ModuleTitleWidth = ModuleWidthHalf;
        static constexpr float ModuleTitleBottom = ModuleTitleY + ModuleTitleHeight;

        static constexpr float ModMatABX = ModuleX + ModMatEditorMargin;
        static constexpr float ModMatABY = ModuleTitleBottom;

        static constexpr float ModMatEditorX = ModMatABX;
        static constexpr float ModMatEditorY = ModMatABY + ModMatABHeight;
        static constexpr float ModMatEditorWidth = ModuleWidth - ModMatEditorMargin * 2.f;
        static constexpr float ModMatEditorHeight = ModuleHeight - ModuleTitleHeight - 5.f - ModMatABHeight - ModParamsTableTopHeight;
        static constexpr float ModMatEditorBottom = ModMatEditorY + ModMatEditorHeight;

        static constexpr float ModParamsTableTopX = ModMatEditorX;
        static constexpr float ModParamsTableTopY = ModMatEditorBottom;
        static constexpr float ModParamsTableTopWidth = ModMatEditorWidth;

        static constexpr float GenAniX = -1 - SidePanelWidth;
        static constexpr float GenAniY = SidePanelMidY;
        static constexpr float GenAniWidth = SidePanelWidth;
        static constexpr float GenAniHeight = SidePanelHeight * .5f;

        static constexpr float TooltipY = -1 - TooltipHeight;

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
        Toast toast;

        enum class kLabels
        {
            kTitle, kDev, kTitleModal, kTitleFlanger, kTitleMacro,
            kEnvAmpAtk, kEnvAmpDcy, kEnvAmpSus, kEnvAmpRls,
            kModalBlend, kModalSpreizung, kModalHarmonie, kModalKraft, kModalReso,
            kModalBlendBreite, kModalSpreizungBreite, kModalKraftBreite,
            kNumLabels
        };
		static constexpr int NumLabels = static_cast<int>(kLabels::kNumLabels);
        std::array<Label, NumLabels> labels;
        enum class kKnobs
        {
            kMacro, kEnvAmpAttack, kEnvAmpDecay, kEnvAmpSustain, kEnvAmpRelease,
			kModalBlend, kModalSpreizung, kModalHarmonie, kModalKraft, kModalReso,
            kModalBlendEnv, kModalSpreizungEnv, kModalHarmonieEnv, kModalKraftEnv,
            kModalBlendBreite, kModalSpreizungBreite, kModalKraftBreite,
            kNumKnobs
        };
		static constexpr int NumKnobs = static_cast<int>(kKnobs::kNumKnobs);
        std::array<Knob, NumKnobs> knobs;
        std::array<ModDial, NumKnobs> modDials;
        enum class kButtons { kMacroRel, kMacroSwap, kMaterialDropDown, kMaterialA, kMaterialB, kNumButtons };
		static constexpr int NumButtons = static_cast<int>(kButtons::kNumButtons);
		std::array<Button, NumButtons> buttons;
		
        std::array<ModalMaterialView, 2> materialViews;
        DropDownMenu materialDropDown;

        //JUCE_HEAVYWEIGHT_LEAK_DETECTOR(Editor)

        Label& get(kLabels) noexcept;
		Knob& get(kKnobs) noexcept;
		ModDial& getModDial(kKnobs) noexcept;
		Button& get(kButtons) noexcept;
    };
}