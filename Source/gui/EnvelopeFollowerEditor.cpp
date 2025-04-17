#include "EnvelopeFollowerEditor.h"

namespace gui
{
	// Visualizer

	EnvelopeFollowerEditor::Visualizer::Visualizer(Utils& u, const EnvFol& envelopeFollower) :
		Comp(u),
		img(),
		y0(0.f)
	{
		setOpaque(true);
		add(Callback([&, &envFol = envelopeFollower]()
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
				const auto meter = static_cast<float>(envFol.getMeter());
				const auto y1 = math::limit(0.f, h, h - meter * h);
				setCol(g, CID::Mod);
				const auto y = std::min(y0, y1);
				const BoundsF rect(x, y, valSize, h - y);
				g.fillRect(rect.toNearestInt());
				y0 = y1;
				repaint();
			}, 0, cbFPS::k60, true));
	}

	void EnvelopeFollowerEditor::Visualizer::resized()
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

	void EnvelopeFollowerEditor::Visualizer::paint(Graphics& g)
	{
		g.drawImageAt(img, 0, 0, false);
	}

	// Editor

	EnvelopeFollowerEditor::EnvelopeFollowerEditor(const EnvFol& envFol, Utils& u,
		PID pGain, PID pAttack, PID pDecay, PID pSmooth) :
		Comp(u),
		visualizer(u, envFol),
		title(u), gainLabel(u), attackLabel(u), decayLabel(u), smoothLabel(u),
		gain(u), attack(u), decay(u), smooth(u),
		gainMod(u), attackMod(u), decayMod(u), smoothMod(u),
		randomizer(u, "randenvfol"),
		labelGroup()
	{
		layout.init
		(
			{ 1, 1, 1, 1 },
			{ 1, 5, 2, 1 }
		);

		addAndMakeVisible(visualizer);
		addAndMakeVisible(title);
		addAndMakeVisible(gainLabel);
		addAndMakeVisible(attackLabel);
		addAndMakeVisible(decayLabel);
		addAndMakeVisible(smoothLabel);
		addAndMakeVisible(gain);
		addAndMakeVisible(attack);
		addAndMakeVisible(decay);
		addAndMakeVisible(smooth);
		addAndMakeVisible(gainMod);
		addAndMakeVisible(attackMod);
		addAndMakeVisible(decayMod);
		addAndMakeVisible(smoothMod);
		addAndMakeVisible(randomizer);

		makeKnob(gain);
		makeKnob(attack);
		makeKnob(decay);
		makeKnob(smooth);

		makeParameter(pGain, gain);
		makeParameter(pAttack, attack);
		makeParameter(pDecay, decay);
		makeParameter(pSmooth, smooth);

		gainMod.attach(pGain);
		attackMod.attach(pAttack);
		decayMod.attach(pDecay);
		smoothMod.attach(pSmooth);

		const auto fontKnobs = font::dosisBold();
		makeTextLabel(gainLabel, "Gain", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(attackLabel, "Attack", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(decayLabel, "Decay", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(smoothLabel, "Smooth", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(title, "Mod Envelope:", font::dosisMedium(), Just::centredLeft, CID::Txt);
		labelGroup.add(gainLabel);
		labelGroup.add(attackLabel);
		labelGroup.add(decayLabel);
		labelGroup.add(smoothLabel);

		randomizer.add(pGain);
		randomizer.add(pAttack);
		randomizer.add(pDecay);
		randomizer.add(pSmooth);
	}

	void EnvelopeFollowerEditor::paint(Graphics& g)
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

	void EnvelopeFollowerEditor::resized()
	{
		const auto thicc = utils.thicc;
		layout.resized(getLocalBounds().toFloat());
		layout.place(title, 0, 0, 2, 1);
		title.setMaxHeight(thicc);
		visualizer.setBounds(layout(0, 1, 4, 1).reduced(thicc).toNearestInt());
		layout.place(gain, 0, 2, 1, 1); locateAtKnob(gainMod, gain);
		layout.place(attack, 1, 2, 1, 1); locateAtKnob(attackMod, attack);
		layout.place(decay, 2, 2, 1, 1); locateAtKnob(decayMod, decay);
		layout.place(smooth, 3, 2, 1, 1); locateAtKnob(smoothMod, smooth);
		layout.place(gainLabel, 0, 3, 1, 1);
		layout.place(attackLabel, 1, 3, 1, 1);
		layout.place(decayLabel, 2, 3, 1, 1);
		layout.place(smoothLabel, 3, 3, 1, 1);
		labelGroup.setMaxHeight();
		layout.place(randomizer, 3, 0, 1, 1);
	}
}