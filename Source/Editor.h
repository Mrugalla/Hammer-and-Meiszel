#pragma once
#include "gui/RadioButton.h"
#include "gui/GenAniComp.h"
#include "gui/ModalParamsEditor.h"
#include "gui/IOEditor.h"
#include "gui/Tooltip.h"
#include "gui/EnvelopeGeneratorEditor.h"
#include "gui/EnvelopeFollowerEditor.h"
#include "gui/RandomizerEditor.h"
#include "gui/hnm/ModalModuleEditor.h"
#include "gui/ColoursEditor.h"
#include "gui/ManifestOfWisdom.h"
#include "gui/Credits.h"
#include "gui/KeySelectorEditor.h"
#include "gui/hnm/TopEditor.h"
#include "gui/LabelPluginRecorder.h"
#include "gui/ParameterEditor.h"

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
        Callback callback;
        Image marbleImg;
        Utils utils;
        Layout layout;
        evt::Member evtMember;
        CompPower compPower;
        Tooltip tooltip;
		ParameterEditor parameterEditor;
        patch::Browser patchBrowser;
        TopEditor topEditor;
        GenAniGrowTrees genAni;
		ModalParamsEditor modParamsEditor;
		IOEditor ioEditor;
        ColoursEditor coloursEditor;
        ManifestOfWisdom manifestOfWisdom;
        Credits credits;
        Toast toast;
        LabelPluginRecorder labelTitle;
        Label labelDev, labelNoiseBlend;
        Knob noiseBlend;
		ModDial modDialNoiseBlend;
        KeySelectorEditor keySelector;
		enum class kEnvGens { kEnvGenAmp, kEnvGenMod, kNumEnvGens };
		static constexpr int NumEnvGens = static_cast<int>(kEnvGens::kNumEnvGens);
        EnvelopeGeneratorMultiVoiceEditor::PIDsTemposync modEnvPIDsSync;
		EnvelopeGeneratorMultiVoiceEditor envGenAmp, envGenMod;
		EnvelopeFollowerEditor envFolMod;
        RandomizerEditor randMod;
        RadioButton modSelect;
		ModalModuleEditor modalEditor;
        ButtonColours buttonColours;
        ButtonWisdom manifestOfWisdomButton;
        ButtonCredits buttonCredits;
    };
}