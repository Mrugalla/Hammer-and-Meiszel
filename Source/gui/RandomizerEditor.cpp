#include "RandomizerEditor.h"

namespace gui
{
	// Visualizer

	RandomizerEditor::Visualizer::Visualizer(Utils& u, const RandMod& randMod) :
		Comp(u),
		img(),
		y0(0.f)
	{
		setOpaque(true);
		add(Callback([&, &rand = randMod]()
			{
				if (!img.isValid())
					return;
				const auto thicc = utils.thicc;
				const auto valSize = std::round(thicc);
				const auto valSizeInt = static_cast<int>(valSize);
				img.moveImageSection(0, 0, valSizeInt, 0, getWidth() - valSizeInt, getHeight());
				Graphics g{ img };
				const auto w = static_cast<float>(getWidth());
				const auto h = static_cast<float>(getHeight());
				const auto x = w - valSize;
				setCol(g, CID::Bg);
				g.fillRect(x, 0.f, valSize, h);
				const auto meter = rand.getMeter();
				const auto y1 = math::limit(0.f, h, h - meter * h);
				setCol(g, CID::Mod);
				const auto y = std::min(y0, y1);
				const BoundsF rect(x, y, valSize, h - y);
				g.fillRect(rect.toNearestInt());
				y0 = y1;
				repaint();
			}, 0, cbFPS::k60, true));
	}

	void RandomizerEditor::Visualizer::resized()
	{
		if (img.isValid())
		{
			img = img.rescaled(getWidth(), getHeight(), Graphics::lowResamplingQuality);
			return;
		}
		img = Image(Image::RGB, getWidth(), getHeight(), false);
		Graphics g{ img };
		g.fillAll(getColour(CID::Bg));
		y0 = static_cast<float>(getHeight());
	}

	void RandomizerEditor::Visualizer::paint(Graphics& g)
	{
		g.drawImageAt(img, 0, 0, false);
	}

	// Editor

	RandomizerEditor::RandomizerEditor(const RandMod& randMod, Utils& u,
		PID pRateSync, PID pSmooth, PID pComplex, PID pDropout) :
		Comp(u),
		visualizer(u, randMod),
		title(u), rateSyncLabel(u), smoothLabel(u), complexLabel(u), dropoutLabel(u),
		rateSync(u), smooth(u), complex(u), dropout(u),
		rateSyncMod(u), smoothMod(u), complexMod(u), dropoutMod(u),
		randomizer(u, "randmod"),
		labelGroup()

	{
		layout.init
		(
			{ 1, 1, 1, 1 },
			{ 1, 5, 2, 1 }
		);

		addAndMakeVisible(visualizer);
		addAndMakeVisible(title);
		addAndMakeVisible(rateSyncLabel); addAndMakeVisible(smoothLabel); addAndMakeVisible(complexLabel); addAndMakeVisible(dropoutLabel);
		addAndMakeVisible(rateSync); addAndMakeVisible(smooth); addAndMakeVisible(complex); addAndMakeVisible(dropout);
		addAndMakeVisible(rateSyncMod); addAndMakeVisible(smoothMod); addAndMakeVisible(complexMod); addAndMakeVisible(dropoutMod);
		addAndMakeVisible(randomizer);

		makeKnob(rateSync);
		makeKnob(smooth);
		makeKnob(complex);
		makeKnob(dropout);

		makeParameter(pRateSync, rateSync);
		makeParameter(pSmooth, smooth);
		makeParameter(pComplex, complex);
		makeParameter(pDropout, dropout);

		rateSyncMod.attach(pRateSync);
		smoothMod.attach(pSmooth);
		complexMod.attach(pComplex);
		dropoutMod.attach(pDropout);

		const auto fontKnobs = font::dosisBold();
		makeTextLabel(rateSyncLabel, "Rate", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(smoothLabel, "Smooth", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(complexLabel, "Complex", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(dropoutLabel, "Dropout", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(title, "Mod Envelope:", font::dosisMedium(), Just::centredLeft, CID::Txt);
		labelGroup.add(rateSyncLabel);
		labelGroup.add(smoothLabel);
		labelGroup.add(complexLabel);
		labelGroup.add(dropoutLabel);

		randomizer.add(pRateSync);
		randomizer.add(pSmooth);
		randomizer.add(pComplex);
		randomizer.add(pDropout);
	}

	void RandomizerEditor::paint(Graphics& g)
	{
		const auto col1 = Colours::c(CID::Bg);
		const auto col2 = Colours::c(CID::Darken);
		const auto thicc = utils.thicc;
		const auto envGenBounds = visualizer.getBounds().toFloat();
		const auto envGenX = envGenBounds.getX();
		const auto envGenY = envGenBounds.getY();
		const auto envGenBtm = envGenBounds.getBottom();
		Gradient gradient(col1, envGenX, envGenY, col2, envGenX, envGenBtm, false);
		g.setGradientFill(gradient);
		g.fillRoundedRectangle(envGenBounds.withBottom(layout.getY(-1)), thicc);
	}

	void RandomizerEditor::resized()
	{
		const auto thicc = utils.thicc;
		layout.resized(getLocalBounds().toFloat());
		layout.place(title, 0, 0, 2, 1);
		title.setMaxHeight(thicc);
		visualizer.setBounds(layout(0, 1, 4, 1).reduced(thicc).toNearestInt());
		layout.place(rateSync, 0, 2, 1, 1); locateAtKnob(rateSyncMod, rateSync);
		layout.place(smooth, 1, 2, 1, 1); locateAtKnob(smoothMod, smooth);
		layout.place(complex, 2, 2, 1, 1); locateAtKnob(complexMod, complex);
		layout.place(dropout, 3, 2, 1, 1); locateAtKnob(dropoutMod, dropout);
		layout.place(rateSyncLabel, 0, 3, 1, 1);
		layout.place(smoothLabel, 1, 3, 1, 1);
		layout.place(complexLabel, 2, 3, 1, 1);
		layout.place(dropoutLabel, 3, 3, 1, 1);
		labelGroup.setMaxHeight();
		layout.place(randomizer, 3, 0, 1, 1);
	}
}