#pragma once
#include "Knob.h"
#include "../audio/dsp/Randomizer.h"
#include "ButtonRandomizer.h"

namespace gui
{
	struct RandomizerEditor :
		public Comp
	{
		using RandMod = dsp::Randomizer;

		struct Visualizer :
			public Comp
		{
			Visualizer(Utils& u, const RandMod& randMod) :
				Comp(u),
				img(),
				y0(0.f)
			{
				setOpaque(true);
				add(Callback([&, &rand = randMod]()
				{
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

			void resized() override
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

			void paint(Graphics& g)
			{
				g.drawImageAt(img, 0, 0, false);
			}
		private:
			Image img;
			float y0;
		};

		RandomizerEditor(const RandMod& randMod, Utils& u, PID pGain, PID pRateSync, PID pSmooth, PID pSpread) :
			Comp(u),
			visualizer(u, randMod),
			title(u), gainLabel(u), rateSyncLabel(u), smoothLabel(u), spreadLabel(u),
			gain(u), rateSync(u), smooth(u), spread(u),
			gainMod(u), rateSyncMod(u), smoothMod(u), spreadMod(u),
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
			addAndMakeVisible(gainLabel);  addAndMakeVisible(rateSyncLabel); addAndMakeVisible(smoothLabel); addAndMakeVisible(spreadLabel);
			addAndMakeVisible(gain); addAndMakeVisible(rateSync); addAndMakeVisible(smooth); addAndMakeVisible(spread);
			addAndMakeVisible(gainMod); addAndMakeVisible(rateSyncMod); addAndMakeVisible(smoothMod); addAndMakeVisible(spreadMod);
			addAndMakeVisible(randomizer);

			makeKnob(gain);
			makeKnob(rateSync);
			makeKnob(smooth);
			makeKnob(spread);

			makeParameter(pGain, gain);
			makeParameter(pRateSync, rateSync);
			makeParameter(pSmooth, smooth);
			makeParameter(pSpread, spread);

			gainMod.attach(pGain);
			rateSyncMod.attach(pRateSync);
			smoothMod.attach(pSmooth);
			spreadMod.attach(pSpread);

			const auto fontKnobs = font::dosisBold();
			makeTextLabel(gainLabel, "Gain", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(rateSyncLabel, "Rate", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(smoothLabel, "Smooth", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(spreadLabel, "Spread", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(title, "Mod Envelope:", font::dosisMedium(), Just::centredLeft, CID::Txt);
			labelGroup.add(rateSyncLabel);

			randomizer.add(pGain);
			randomizer.add(pRateSync);
			randomizer.add(pSmooth);
			randomizer.add(pSpread);
		}

		void paint(Graphics& g)
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

		void resized() override
		{
			const auto thicc = utils.thicc;
			layout.resized(getLocalBounds().toFloat());
			layout.place(title, 0, 0, 2, 1);
			title.setMaxHeight(thicc);
			visualizer.setBounds(layout(0, 1, 4, 1).reduced(thicc).toNearestInt());
			layout.place(gain, 0, 2, 1, 1); locateAtKnob(gainMod, gain);
			layout.place(rateSync, 1, 2, 1, 1); locateAtKnob(rateSyncMod, rateSync);
			layout.place(smooth, 2, 2, 1, 1); locateAtKnob(smoothMod, smooth);
			layout.place(spread, 3, 2, 1, 1); locateAtKnob(spreadMod, spread);
			layout.place(gainLabel, 0, 3, 1, 1);
			layout.place(rateSyncLabel, 1, 3, 1, 1);
			layout.place(smoothLabel, 2, 3, 1, 1);
			layout.place(spreadLabel, 3, 3, 1, 1);
			labelGroup.setMaxHeight();
			layout.place(randomizer, 3, 0, 1, 1);
		}

	private:
		Visualizer visualizer;
		Label title, gainLabel, rateSyncLabel, smoothLabel, spreadLabel;
		Knob gain, rateSync, smooth, spread;
		ModDial gainMod, rateSyncMod, smoothMod, spreadMod;
		ButtonRandomizer randomizer;
		LabelGroup labelGroup;
	};
}