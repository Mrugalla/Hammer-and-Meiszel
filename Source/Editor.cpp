#include "Editor.h"

namespace gui
{
    evt::Evt makeEvt(Editor& editor)
    {
        return [&e = editor](evt::Type type, const void*)
        {
            switch (type)
            {
            case evt::Type::ClickedEmpty:
				e.coloursEditor.setVisible(false);
                e.manifestOfWisdom.setVisible(false);
                e.buttonColours.value = 0.f;
				e.manifestOfWisdomButton.value = 0.f;
                e.buttonColours.repaint();
				e.manifestOfWisdomButton.repaint();
                e.giveAwayKeyboardFocus();
                return;
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
        marbleImg(juce::ImageCache::getFromMemory(BinaryData::marble_png, BinaryData::marble_pngSize)),
        utils(*this, p),
        layout(),
        evtMember(utils.eventSystem, makeEvt(*this)),
        compPower(utils),
        tooltip(utils),
        buttonRandomizer(utils, "randomizer"),
        buttonSoftClip(utils),
        genAni(utils),
        modParamsEditor(utils),
        ioEditor(utils),
        coloursEditor(utils),
        manifestOfWisdom(utils),
        toast(utils),
        labelDev(utils),
        labelTitle(utils),
        labelNoiseBlend(utils),
        noiseBlend(utils),
		modDialNoiseBlend(utils),
        keySelector(utils, utils.audioProcessor.pluginProcessor.keySelector),
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
        modalEditor(utils),
        buttonColours(coloursEditor),
        manifestOfWisdomButton(utils, manifestOfWisdom)
    {
        layout.init
        (
            { 5, 13, 5 },
            { 2, 2, 13, 1, 1 }
        );
        addAndMakeVisible(labelDev);
        addAndMakeVisible(labelTitle);
        addAndMakeVisible(tooltip);
		addAndMakeVisible(buttonRandomizer);
		addAndMakeVisible(buttonSoftClip);
        addAndMakeVisible(genAni);
        addAndMakeVisible(ioEditor);
        addAndMakeVisible(modParamsEditor);
		addAndMakeVisible(labelNoiseBlend);
		addAndMakeVisible(noiseBlend);
		addAndMakeVisible(modDialNoiseBlend);
		addAndMakeVisible(keySelector);
		for (auto& envGen : envGens)
			addAndMakeVisible(envGen);
		addAndMakeVisible(modalEditor);
        addAndMakeVisible(buttonColours);
        addAndMakeVisible(manifestOfWisdomButton);
		addChildComponent(coloursEditor);
		addChildComponent(manifestOfWisdom);
        addChildComponent(toast);
        addAndMakeVisible(compPower);

        {
            makeParameter(buttonSoftClip, PID::SoftClip, Button::Type::kToggle, makeButtonOnPaintClip());
            for (auto i = 0; i < param::NumParams; ++i)
            {
                const auto pID = static_cast<PID>(i);
				if (pID != PID::KeySelectorEnabled && pID != PID::NoiseBlend)
                    buttonRandomizer.add(pID);
            }
            buttonRandomizer.add([&](ButtonRandomizer::Randomizer& rand)
            {
                auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& xenManager = utils.audioProcessor.xenManager;

                for (auto i = 0; i < 2; ++i)
                {
                    auto& mat = modalFilter.getMaterial(i);
                    if (mat.status == dsp::modal::StatusMat::Processing)
                    {
                        std::array<double, dsp::modal::NumFilters> ratios;
                        ratios[0] = 1.;
						for (auto j = 1; j < dsp::modal::NumFilters; ++j)
							ratios[j] = 1. + static_cast<double>(rand()) * 32.;
                        std::sort(ratios.begin(), ratios.end());

						auto& peaks = mat.peakInfos;

						peaks[0].mag = static_cast<double>(rand());
						peaks[0].ratio = ratios[0];
						peaks[0].freqHz = xenManager.noteToFreqHz(static_cast<double>(rand()) * 127.);
                        peaks[0].keytrack = 0.;

                        for (auto j = 1; j < dsp::modal::NumFilters; ++j)
                        {
                            auto& peak = peaks[j];
                            peak.mag = static_cast<double>(rand());
							peak.ratio = ratios[j];
							peak.freqHz = xenManager.noteToFreqHz(static_cast<double>(rand()) * 127.);
							peak.keytrack = static_cast<double>(rand());
                        }

                        mat.reportUpdate();
                    }
                }
            });
        }

        {
            makeSlider(noiseBlend, true);
            makeParameter(PID::NoiseBlend, noiseBlend, false);
			modDialNoiseBlend.attach(PID::NoiseBlend);
			modDialNoiseBlend.verticalDrag = false;
			makeTextLabel(labelNoiseBlend, "Noise", font::dosisMedium(), Just::centred, CID::Txt);
        }

        {
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
        }

        loadBounds(*this);
        utils.audioProcessor.pluginProcessor.editorExists.store(true);
        setOpaque(true);
    }

    Editor::~Editor()
    {
        utils.audioProcessor.pluginProcessor.editorExists.store(false);
    }
    
    void Editor::paint(Graphics& g)
    {
        g.drawImageAt(marbleImg, 0, 0, false);
        g.setColour(Colours::c(CID::Bg).withMultipliedAlpha(.85f));
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
        e.layout.place(e.labelNoiseBlend, 0.f, 1, .2f, 1);
        e.labelNoiseBlend.setMaxHeight();
		e.layout.place(e.noiseBlend,      .2f, 1, .7f, 1);
        locateAtSlider(e.modDialNoiseBlend, e.noiseBlend);
        const auto envsArea = e.layout(0, 2, 1, 1);
		const auto x = envsArea.getX();
		auto y = envsArea.getY();
		const auto w = envsArea.getWidth();
		const auto h = envsArea.getHeight();
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

        marbleImg = juce::ImageCache::getFromMemory(BinaryData::marble_png, BinaryData::marble_pngSize).rescaled(getWidth(), getHeight());
		fixStupidJUCEImageThingie(marbleImg);
        for (auto y = 0; y < marbleImg.getHeight(); ++y)
            for (auto x = 0; x < marbleImg.getWidth(); ++x)
            {
				auto pxl = marbleImg.getPixelAt(x, y);
                marbleImg.setPixelAt(x, y, pxl.withBrightness(1.f - pxl.getBrightness()));
            }

        compPower.setBounds(getLocalBounds());

        utils.resized();
        const auto thicc = utils.thicc;
		const auto thicc2 = thicc * 2.f;

        layout.resized(getLocalBounds());

        // top panel
		layout.place(labelTitle, 0, 0.f, 1, .5f);
        labelTitle.setMaxHeight();
		layout.place(labelDev, 0, .5f, 1, .5f);
        labelDev.setMaxHeight();
        // top mid panel
        // leave some space for the preset browser
		layout.place(buttonRandomizer, 1.5f, 0, .25f, 1);
		layout.place(buttonSoftClip, 1.75f, 0, .25f, 1);

        // left panel
        resizeLeftPanel(*this, thicc2);
        
        // right panel
        layout.place(ioEditor, 2, 0, 1, 3);
        layout.place(genAni, 2, 1.5f, 1, 1.5f);
        layout.place(buttonColours, 2.f, -3, .5f, 1);
		layout.place(manifestOfWisdomButton, 2.5f, -3, .5f, 1);

        // center panel
        layout.place(keySelector, 1, 1, 1, 1);
        layout.place(modalEditor, 1, 2, 1, 2);
        layout.place(coloursEditor, 1, 2, 1, 1);
        layout.place(manifestOfWisdom, 1, 2, 1, 1);

        // bottom panel
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