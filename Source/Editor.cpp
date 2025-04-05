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
		credits(utils),
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
        manifestOfWisdomButton(utils, manifestOfWisdom),
		buttonCredits(utils, credits)
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
		addAndMakeVisible(buttonCredits);
		addChildComponent(coloursEditor);
		addChildComponent(manifestOfWisdom);
		addChildComponent(credits);
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

        const auto sz = String(juce::CharPointer_UTF8("\xc3\x9f"));
        const String hnm("Hammer & Mei" + sz + "el");

        {
            String dev(JucePlugin_Manufacturer);
            auto titleFont = font::flx();
            const auto devFont = titleFont;
            titleFont.setExtraKerningFactor(.2f);
            makeTextLabel(labelDev, "gebastelt von Florian " + dev, devFont, Just::centred, CID::Txt);
            makeTextLabel(labelTitle, hnm, titleFont, Just::centred, CID::Txt);
        }

        {
            credits.add(BinaryData::credits01_png, BinaryData::credits01_pngSize,
                "Welcome to the credits of " + hnm +"!:)\nHere you can find artwork, sketches, development details\nand ofc aknowledgements for my awesome beta team.\n\nUse the left and right arrow buttons to navigate the pages.\nDrag the images to zoom into them!\nClick on the Credits button again to close the credits page!");

            credits.add(BinaryData::credits02_png, BinaryData::credits02_pngSize,
                "A modal filter consists of parallel resonators\nthat resemble a sound's spectral response.\nThey are often used to physically model real instruments\nbut I just wanted " + hnm + " to analyze imported sounds\nso that you can generate your own modal materials!");

			credits.add(BinaryData::credits03_png, BinaryData::credits03_pngSize,
				"My initial plan was to let you capture multiple cue points\nfor each imported sample, so that an envelope can modulate over it.\nIt likely gets you close to the input sample!\nBut it would essentially be a multidimensional modal material.\nA lot of stuff to superwise while adjusting a patch.");

            credits.add(BinaryData::credits04_png, BinaryData::credits04_pngSize,
                "Every one of the 15 voices has a chain\ncomposed of keytracked effects.\nI considered a lot of different ones at first.\nMaybe they could be arranged horizontally\nwith a big pitch knob to transpose everything.");

            credits.add(BinaryData::credits05_png, BinaryData::credits05_pngSize,
                "Blending 2 materials is basically like a multi-cue material,\nbut less forceful.\nVarious warp modes are used to twist the mixed material.\nAn envelope modulates them to add movement.");

            credits.add(BinaryData::credits06_png, BinaryData::credits06_pngSize,
                "Challenging to squeeze so many ideas into a small module\nbut that's required to keep the signal chain workflow like that.\nThere could be many processors besides the modal filter\nThey will define the essence of the effect.");

            credits.add(BinaryData::credits07_png, BinaryData::credits07_pngSize,
                "I considered a vertical layout, where each modal material\ncan be expanded for fine adjustments.\nThe other processors (comb, allpass etc.) are subordinate to the modal filter.\nI suppose that would feel more laidback.");

            credits.add(BinaryData::credits08_png, BinaryData::credits08_pngSize,
                "There were times when I didn't even understand my own code\nI had to draw weird diagrams to keep an overview of the processes.\nI hope I was able to eliminate all bugs, but if I didn't\nplease report them to me in my Discord group.");

			credits.add(BinaryData::credits09_png, BinaryData::credits09_pngSize,
				"A single mod envelope can modulate all of the effects.\nThere only need to be 2 distinct, but well designed filters.\nThe modal filter and a keytracked flanger.\nI would have to rewrite the rap now if I wanted to use it.");

			credits.add(BinaryData::credits10_png, BinaryData::credits10_pngSize,
				"'Die Wahre Sahne kommt von Allpass Filtern im Feedback Loop.'\n\nIs that so? They turned out to be rather unpredictable at times.\nBut there is certain potential in the idea.\nIt needs to be investigated later.");

			credits.add(BinaryData::credits11_png, BinaryData::credits11_pngSize,
				"The flanger could layer comb filters to play the scale of the entire music\nwhile a formant filter carves a voice out of the resonances.\nSounds awesome, but that's a lot of parameters.");

			credits.add(BinaryData::credits12_png, BinaryData::credits12_pngSize,
				"Maybe if they are arranged as a cube\n3 dimensions of parameters squeezed down in a beautiful way.\nLooks kinda impractical though\nespecially if you also want to visualize modulation.\nThat's a challenge for another project. :)");

            credits.add(BinaryData::credits13_png, BinaryData::credits13_pngSize,
                "Every modal parameter has an envelope depth, and stereo width.\nThe stereo width can be l/r or m/s.\nThe number of parameters exploded once again and I designed a tabbed window.\nI don't like faders though..");

            credits.add(BinaryData::credits14_png, BinaryData::credits14_pngSize,
                "Creating modal materials is the heart of the plugin\nso there need to be varying fun ways to generate those.\nI don't expect anyone to ever manually touch a partial in this plugin tbh.\nKnobs required tabbing the parameter dimensions though.");

            credits.add(BinaryData::credits16_png, BinaryData::credits16_pngSize,
                "The tab buttons for the main value (M)\nenvelope depth (E) and stereo width (W) are on the right.\nThe effect chain is below the selected module.\nThe space for the modal parameter knobs is rather limited in height.");

            credits.add(BinaryData::credits15_png, BinaryData::credits15_pngSize,
                "If there is a triangle of parameters for the prameter dimensions\nI don't have to tab them.\nI was working on the comb filter with the allpass filters in the fb loop.\nIt was all about experimenting with different textures, but very vague and sharp.");

            credits.add(BinaryData::credits17_png, BinaryData::credits17_pngSize,
                "If the formant filter runs in parallel with the modal filter\nyou can add fixed frequency partials to the keytracked ones.\nBefore this, every partial had its own keytrack amount\nand fixed frequency value.\nIt required each partial to be 4dimensional and because of that\ntoo hard to find a good workflow for so far.");

            credits.add("A lot of the changes I made throughout the project\nwould not have been possible without my beta team.\n\nThank you for your voluntary work!\nI don't take that for granted,\nbecause it means that my work also means something to you!\n\nI hope I can also lead sounddesign enthusiasts to your work! :)");

            credits.init();
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
        layout.place(buttonColours,             2.f, -3, .333f, 1);
		layout.place(manifestOfWisdomButton,    2.333f, -3, .333f, 1);
		layout.place(buttonCredits,             2.667f, -3, .333f, 1);

        // center panel
        layout.place(keySelector, 1, 1, 1, 1);
		const auto centerPaneLBounds = layout(1, 2, 1, 2).toNearestInt();
		modalEditor.setBounds(centerPaneLBounds);
		coloursEditor.setBounds(centerPaneLBounds);
		manifestOfWisdom.setBounds(centerPaneLBounds);
		patchBrowser.setBounds(centerPaneLBounds);
		credits.setBounds(centerPaneLBounds);

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