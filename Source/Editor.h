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
            kNumLabels
        };
		static constexpr int NumLabels = static_cast<int>(kLabels::kNumLabels);
        std::array<Label, NumLabels> labels;
        enum class kKnobs
        {
            kMacro, kEnvAmpAttack, kEnvAmpDecay, kEnvAmpSustain, kEnvAmpRelease,
			kModalBlend, kModalSpreizung, kModalHarmonie, kModalKraft, kModalReso,
            kModalBlendEnv, kModalSpreizungEnv, kModalHarmonieEnv, kModalKraftEnv,
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