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
        
        painterMacro(true),
        macroKnob(utils),
        materialView(utils, utils.audioProcessor.pluginProcessor.modalFilter)
    {
        layout.init
        (
            { 1, 3, 13, 1 },
            { 1, 2, 5, 2, 2, 1 }
        );

        addAndMakeVisible(bgImage);
        addAndMakeVisible(bgImage.refreshButton);
        addAndMakeVisible(tooltip);

        makeTextLabel(labels[kDev], JucePlugin_Manufacturer, font::flx(), Just::centred, CID::Txt);

        auto sz = String(juce::CharPointer_UTF8("\xc3\x9f"));
        String name(JucePlugin_Name);
        for (auto i = 0; i < name.length(); ++i)
            if (name[i] == 's' && name[i + 1] == 'z')
                name = name.replaceSection(i, 2, sz);
        makeTextLabel(labels[kTitle], name, font::flx(), Just::centred, CID::Txt);
        for (auto& label : labels)
            addAndMakeVisible(label);

        addAndMakeVisible(macroKnob);
        macroKnob.attachParameter("Macro", PID::Macro, &painterMacro);
        
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

        layout.place(labels[kDev], 1, 1, 1, 1);
        layout.place(labels[kTitle], 2, 1, 1, 1);
        setMaxCommonHeight(labels.data(), kNumLabels);

        layout.place(macroKnob, 1, 2, 1, 2, false);
        layout.place(macroKnob.label, 1, 4, 1, 1, false);

        layout.place(materialView, 2, 2, 1, 1, false);

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