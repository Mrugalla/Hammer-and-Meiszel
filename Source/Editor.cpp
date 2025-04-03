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
                e.patchBrowser.setVisible(false);
                e.buttonColours.value = 0.f;
				e.manifestOfWisdomButton.value = 0.f;
                e.buttonColours.repaint();
				e.manifestOfWisdomButton.repaint();
                e.parameterEditor.setActive(false);
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
        callback(Callback([&]()
        {
			auto& modSelectParam = utils.getParam(PID::ModSelect);
			const auto val = static_cast<int>(std::round(modSelectParam.getValModDenorm()));
            envGenMod.setVisible(val == 0);
			envFolMod.setVisible(val == 1);
			randMod.setVisible(val == 2);
        }, 0, cbFPS::k15, true)),
        marbleImg(juce::ImageCache::getFromMemory(BinaryData::marble_png, BinaryData::marble_pngSize)),
        utils(*this, p),
        layout(),
        evtMember(utils.eventSystem, makeEvt(*this)),
        compPower(utils),
        tooltip(utils),
        parameterEditor(utils),
        patchBrowser(utils),
		topEditor(utils, patchBrowser),
        genAni(utils),
        modParamsEditor(utils),
        ioEditor(utils),
        coloursEditor(utils),
        manifestOfWisdom(utils),
        toast(utils),
        labelTitle(utils),
        labelDev(utils),
        labelNoiseBlend(utils),
        noiseBlend(utils),
		modDialNoiseBlend(utils),
        keySelector(utils, utils.audioProcessor.pluginProcessor.keySelector),
		modEnvPIDsSync(PID::EnvGenModAttackTS, PID::EnvGenModDecayTS, PID::EnvGenModReleaseTS, PID::EnvGenModTemposync),
        envGenAmp
        (
            utils, "Gain Envelope:",
            PID::EnvGenAmpAttack, PID::EnvGenAmpDecay,
            PID::EnvGenAmpSustain, PID::EnvGenAmpRelease
        ),
        envGenMod
        (
            utils, "Mod Envelope:",
            PID::EnvGenModAttack, PID::EnvGenModDecay,
            PID::EnvGenModSustain, PID::EnvGenModRelease,
            &modEnvPIDsSync
        ),
		envFolMod
        (
            p.pluginProcessor.envFolMod, utils,
            PID::EnvFolModGain, PID::EnvFolModAttack,
            PID::EnvFolModDecay, PID::EnvFolModSmooth
        ),
        randMod
        (
			p.pluginProcessor.randMod, utils,
            PID::RandModGain,
			PID::RandModRateSync,
			PID::RandModSmooth,
			PID::RandModSpread
        ),
        modSelect(utils),
        modalEditor(utils),
        buttonColours(coloursEditor),
        manifestOfWisdomButton(utils, manifestOfWisdom)
    {
        layout.init
        (
            { 5, 13, 5 },
            { 1, 1, 13, 1, 1 }
        );
        addAndMakeVisible(labelDev);
        addAndMakeVisible(labelTitle);
        addAndMakeVisible(tooltip);
        addAndMakeVisible(topEditor);
        addAndMakeVisible(genAni);
        addAndMakeVisible(ioEditor);
        addAndMakeVisible(modParamsEditor);
		addAndMakeVisible(labelNoiseBlend);
		addAndMakeVisible(noiseBlend);
		addAndMakeVisible(modDialNoiseBlend);
		addAndMakeVisible(keySelector);
		addAndMakeVisible(envGenAmp);
		addChildComponent(envGenMod);
		addChildComponent(envFolMod);
		addChildComponent(randMod);
		addAndMakeVisible(modSelect);
		addAndMakeVisible(modalEditor);
        addAndMakeVisible(buttonColours);
        addAndMakeVisible(manifestOfWisdomButton);
		addChildComponent(coloursEditor);
		addChildComponent(manifestOfWisdom);
        addChildComponent(patchBrowser);
        addChildComponent(toast);
        addChildComponent(parameterEditor);
        addAndMakeVisible(compPower);

        {
            modSelect.attach(PID::ModSelect);
            utils.add(&callback);
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
            makeTextLabel(labelDev, "gebastelt von Florian " + dev, devFont, Just::centred, CID::Txt);

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

        setMouseCursor(makeCursor());
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
        const auto noiseSize = .1f;
        e.layout.place(e.labelNoiseBlend, 0.f, 2.f, .2f, noiseSize);
        e.labelNoiseBlend.setMaxHeight(e.utils.thicc);
		e.layout.place(e.noiseBlend,      .2f, 2.f, .7f, noiseSize);
        locateAtSlider(e.modDialNoiseBlend, e.noiseBlend);
		const auto modSelectH = .5f;
		const auto envsY = 2.f + noiseSize;
		const auto envsHMoar = 2.f - noiseSize * 2.f;
        const auto envsH = envsHMoar - modSelectH;
        const auto envsArea = e.layout(0, envsY, 1, envsH);
		const auto x = envsArea.getX();
		auto y = envsArea.getY();
		const auto w = envsArea.getWidth();
		const auto h = envsArea.getHeight();
		const auto hHalf = h * .5f;
        e.envGenAmp.setBounds(BoundsF(x, y, w, hHalf).reduced(envGenMargin).toNearestInt());
        y += hHalf;
        const auto modulatorBounds = BoundsF(x, y, w, hHalf).reduced(envGenMargin).toNearestInt();
		e.envGenMod.setBounds(modulatorBounds);
		e.envFolMod.setBounds(modulatorBounds);
		e.randMod.setBounds(modulatorBounds);
        y += hHalf;
        e.modSelect.setBounds(BoundsF(x, y, w, e.layout.getY(4.f) - y).toNearestInt());
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

        layout.resized(getLocalBounds());

        // top panel
		layout.place(labelTitle, 0, 0, 1, 1);
        labelTitle.setMaxHeight();
		layout.place(labelDev, 0, 1, 1, 1);
        labelDev.setMaxHeight();
        layout.place(topEditor, 1, 0, 1, 1);

        // left panel
        resizeLeftPanel(*this, thicc);
        
        // right panel
        layout.place(ioEditor, 2, 0, 1, 3);
        layout.place(genAni, 2, 2.5f, 1, .5f);
        layout.place(buttonColours, 2.f, -3, .5f, 1);
		layout.place(manifestOfWisdomButton, 2.5f, -3, .5f, 1);

        // center panel
        layout.place(keySelector, 1, 1, 1, 1);
        layout.place(modalEditor, 1, 2, 1, 2);
        layout.place(coloursEditor, 1, 2, 1, 2);
        layout.place(manifestOfWisdom, 1, 2, 1, 2);
		layout.place(patchBrowser, 1, 2, 1, 2);

        // bottom panel
        tooltip.setBounds(layout.bottom().toNearestInt());

        const auto toastWidth = static_cast<int>(utils.thicc * 28.f);
        const auto toastHeight = toastWidth * 3 / 4;
        toast.setSize(toastWidth, toastHeight);
		parameterEditor.setSize(toastWidth * 3, toastHeight);
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