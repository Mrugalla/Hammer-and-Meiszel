#pragma once
#include "gui/GenAniComp.h"
#include "gui/ModalParamsEditor.h"
#include "gui/IOEditor.h"
#include "gui/Tooltip.h"
#include "gui/EnvelopeGeneratorEditor.h"
#include "gui/ModalModuleEditor.h"

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
		ModalParamsEditor modParamsEditor;
		IOEditor ioEditor;
        Toast toast;
        Label labelDev, labelTitle;
		enum class kEnvGens { kEnvGenAmp, kEnvGenMod, kNumEnvGens };
		static constexpr int NumEnvGens = static_cast<int>(kEnvGens::kNumEnvGens);
		std::array<EnvelopeGeneratorMultiVoiceEditor, NumEnvGens> envGens;
		ModalModuleEditor modalEditor;
    };
}