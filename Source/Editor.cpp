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

    Editor::Editor(Processor& p) :
        AudioProcessorEditor(&p),
        audioProcessor(p),
        utils(*this, p),
        layout(),
        evtMember(utils.eventSystem, makeEvt(*this)),
        bgImage(utils),
        tooltip(utils),
        labels
        {
            Label(utils, false),
            Label(utils, false)
        },
        knobs
        {
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils),
            KnobParam(utils)
        },
        painters
        {
            KnobPainterBasic(true),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false),
            KnobPainterBasic(false)
        },
        materialViews
        {
            ModalMaterialView(utils, utils.audioProcessor.pluginProcessor.modalFilter.material.materials[0]),
            ModalMaterialView(utils, utils.audioProcessor.pluginProcessor.modalFilter.material.materials[1]),
        }
    {
        layout.init
        (
            { 1, 5, 2, 3, 2, 5, 1 },
            { 1, 5, 1, 1, 8, 3, 5, 5, 1 }
        );

        addAndMakeVisible(bgImage);
        addAndMakeVisible(bgImage.refreshButton);

        bgImage.createImageFunc = [&](Image& img)
        {
            Graphics g{ img };
            g.fillAll(getColour(CID::Bg));

            auto titleArea = layout(2, 1, 3, 1);
            g.setColour(Colours::c(CID::Interact));
            visualizeGroupNEL(g, titleArea, utils.thicc);
        };
        
        addAndMakeVisible(tooltip);

        String dev(JucePlugin_Manufacturer);
        auto devFont = font::flx();
        devFont.setExtraKerningFactor(.1f);
        makeTextLabel(labels[kDev], "- " + dev + " -", devFont, Just::centred, CID::Txt);

        auto sz = String(juce::CharPointer_UTF8("\xc3\x9f"));
        String name(JucePlugin_Name);
        for (auto i = 0; i < name.length(); ++i)
            if (name[i] == 's' && name[i + 1] == 'z')
                name = name.replaceSection(i, 2, sz);
            else if(name[i] == ' ')
                name = name.replaceSection(i, 1, "\n");
        auto titleFont = font::flx();
        makeTextLabel(labels[kTitle], name, titleFont, Just::centred, CID::Txt);
        for (auto& label : labels)
            addAndMakeVisible(label);

        for(auto& knob: knobs)
            addAndMakeVisible(knob);
        knobs[kMacro].attachParameter("Macro", PID::Macro, &painters[0]);
        knobs[kOct].attachParameter("Res", PID::Oct, &painters[1]);
        knobs[kSemi].attachParameter("Semi", PID::Semi, &painters[2]);
        knobs[kModalReso].attachParameter("Resonanz", PID::ModalResonanz, &painters[3]);
        knobs[kModalHarm].attachParameter("Harmonie", PID::ModalHarmonie, &painters[4]);
        knobs[kModalTon].attachParameter("Tonalitaet", PID::ModalTonalitaet, &painters[5]);
        knobs[kModalMix].attachParameter("Mix", PID::ModalMix, &painters[6]);
        knobs[kCombFeedback].attachParameter("Rueckkopplung", PID::CombRueckkopplung, &painters[7]);
        knobs[kGainDry].attachParameter("Dry", PID::GainDry, &painters[8]);
        knobs[kGainWet].attachParameter("Wet", PID::GainWet, &painters[9]);
        knobs[kGainOut].attachParameter("Out", PID::GainOut, &painters[10]);
        
        for(auto& materialView: materialViews)
			addAndMakeVisible(materialView);

        const auto& user = *audioProcessor.state.props.getUserSettings();
        const auto editorWidth = user.getDoubleValue("EditorWidth", EditorWidth);
        const auto editorHeight = user.getDoubleValue("EditorHeight", EditorHeight);
        setOpaque(true);
        setResizable(true, true);
        setSize
        (
            static_cast<int>(editorWidth),
            static_cast<int>(editorHeight)
        );
    }
    
    void Editor::paintOverChildren(Graphics& g)
    {
        layout.paint(g, getColour(CID::Hover));
    }

    void Editor::resized()
    {
        if (getWidth() < EditorMinWidth)
            return setSize(EditorMinWidth, getHeight());
        if (getHeight() < EditorMinHeight)
            return setSize(getWidth(), EditorMinHeight);

        utils.resized();
        layout.resized(getLocalBounds());

        bgImage.setBounds(getLocalBounds());
        bgImage.refreshButton.setBounds(layout.cornerTopRight().toNearestInt());

        tooltip.setBounds(layout.bottom().toNearestInt());

        layout.place(labels[kTitle], 2, 1, 3, 1);
        layout.place(labels[kDev], 2, 2, 3, 1);
        labels[kTitle].setMaxHeight();
        labels[kDev].setMaxHeight();

        layout.place(knobs[kOct], 2, 3, 1, 1);
        layout.place(knobs[kSemi], 3, 3, 1, 1);

        layout.place(materialViews[0], 1, 4, 1, 1);
        layout.place(materialViews[1], 5, 4, 1, 1);

        layout.place(knobs[kModalHarm], 2, 4.f, 1, .6f);
        layout.place(knobs[kModalReso], 3, 4.f, 1, .6f);
        layout.place(knobs[kModalTon], 4, 4.f, 1, .6f);
        layout.place(knobs[kModalHarm].label, 2, 4.6f, 1, .4f);
        layout.place(knobs[kModalReso].label, 3, 4.6f, 1, .4f);
        layout.place(knobs[kModalTon].label, 4, 4.6f, 1, .4f);

        layout.place(knobs[kModalMix], 2, 5.f, 3, .6f);
        layout.place(knobs[kModalMix].label, 2, 5.6f, 3, .4f);

        layout.place(knobs[kCombFeedback], 1, 6.f, 1, .7f);
        layout.place(knobs[kCombFeedback].label, 1, 6.7f, 1, .3f);

        layout.place(knobs[kMacro], 1, 7.f, 1, .7f);
        layout.place(knobs[kMacro].label, 1, 7.7f, 1, .3f);

        layout.place(knobs[kGainDry], 5.f, 7.f, .7f, .333f);
        layout.place(knobs[kGainDry].label, 5.7f, 7.f, .3f, .333f);
        layout.place(knobs[kGainWet], 5.f, 7.333f, .7f, .333f);
        layout.place(knobs[kGainWet].label, 5.7f, 7.333f, .3f, .333f);
        layout.place(knobs[kGainOut], 5.f, 7.666f, .7f, .333f);
        layout.place(knobs[kGainOut].label, 5.7f, 7.666f, .3f, .333f);

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