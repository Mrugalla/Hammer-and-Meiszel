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
        modParamsEditor(utils),
        ioEditor(utils),
        toast(utils),
        labelDev(utils),
		labelTitle(utils),
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
		modalEditor(utils)
    {
        layout.init
        (
            { 5, 13, 5 },
            { 2, 13, 2, 1 }
        );
        addAndMakeVisible(labelDev);
        addAndMakeVisible(labelTitle);
        addAndMakeVisible(tooltip);
        addAndMakeVisible(genAni);
        addAndMakeVisible(ioEditor);
        addAndMakeVisible(modParamsEditor);
		for (auto& envGen : envGens)
			addAndMakeVisible(envGen);
		addAndMakeVisible(modalEditor);
        addChildComponent(toast);

        String dev(JucePlugin_Manufacturer);
        auto titleFont = font::flx();
        const auto devFont = titleFont;
        titleFont.setExtraKerningFactor(.2f);
        makeTextLabel(labelDev, "crafted by Florian " + dev, devFont, Just::centred, CID::Txt);

        auto sz = String(juce::CharPointer_UTF8("\xc3\x9f"));
        String name(JucePlugin_Name);
        for (auto i = 0; i < name.length(); ++i)
            if (name[i] == 's' && name[i + 1] == 'z')
                name = name.replaceSection(i, 2, sz);
        makeTextLabel(labelTitle, name, titleFont, Just::centred, CID::Txt);

        loadBounds(*this);
        utils.audioProcessor.pluginProcessor.editorExists.store(true);
    }

    Editor::~Editor()
    {
        utils.audioProcessor.pluginProcessor.editorExists.store(false);
    }
    
    void Editor::paint(Graphics& g)
    {
        setCol(g, CID::Bg);
        g.fillAll();
    }

    void Editor::paintOverChildren(Graphics&)
    {
       //layout.paint(g, Colour(0x11ffffff));
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
        const auto leftArea = e.layout(0, 1, 1, 2);
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

	void resizeRightPanel(Editor& e)
    {
		const auto rightArea = e.layout(2, 1, 1, 2).toNearestInt();
        e.genAni.setBounds(rightArea);
        e.ioEditor.setBounds(rightArea);
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

		layout.place(labelTitle, 0, 0.f, 1, .5f);
        labelTitle.setMaxHeight();
		layout.place(labelDev, 0, .5f, 1, .5f);
        labelDev.setMaxHeight();

        resizeLeftPanel(*this, thicc2);
		resizeRightPanel(*this);

        layout.place(modalEditor, 1, 1, 1, 1);
        tooltip.setBounds(layout.bottom().toNearestInt());

        const auto toastWidth = static_cast<int>(utils.thicc * 28.f);
        const auto toastHeight = toastWidth * 3 / 4;
        toast.setSize(toastWidth, toastHeight);
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