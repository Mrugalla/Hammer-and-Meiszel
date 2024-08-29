#pragma once
#include "Knob.h"
#include "../audio/dsp/EnvelopeGenerator.h"
#include "Ruler.h"

namespace gui
{
	struct EnvelopeGeneratorMultiVoiceEditor :
		public Comp
	{
		using EnvGen = dsp::EnvelopeGenerator;
		using EnvGenMultiVoice = dsp::EnvGenMultiVoice;

		enum { kAttack, kDecay, kSustain, kRelease, kNumParameters };
		static constexpr int kTitle = kNumParameters;

		struct EnvGenView :
			public Comp
		{
			EnvGenView(Utils& u, PID atkPID, PID dcyPID, PID susPID, PID rlsPID) :
				Comp(u),
				atkParam(u.audioProcessor.params(atkPID)),
				dcyParam(u.audioProcessor.params(dcyPID)),
				susParam(u.audioProcessor.params(susPID)),
				rlsParam(u.audioProcessor.params(rlsPID)),
				ruler(u),
				curve(), curveMod(),
				atkV(-1.f), dcyV(-1.f), susV(-1.f), rlsV(-1.f),
				atkModV(-1.f), dcyModV(-1.f), susModV(-1.f), rlsModV(-1.f)
			{
				layout.init
				(
					{ 1 },
					{ 2, 13 }
				);

				initRuler();

				add(Callback([&]()
				{
					if(updateCurve())
						repaint();
				}, 0, cbFPS::k30, true));
			}

			void resized() override
			{
				layout.resized(getLocalBounds().toFloat());
				layout.place(ruler, 0, 0, 1, 1);
				curve.resize(getWidth());
				curveMod.resize(getWidth());
				updateCurve();
			}

			void paint(Graphics& g) override
			{
				const auto width = getWidth();
				setCol(g, CID::Mod);
				for (auto x = 0; x < width; ++x)
					g.fillRect(curveMod[x]);
				setCol(g, CID::Interact);
				for (auto x = 0; x < width; ++x)
					g.fillRect(curve[x]);
			}

		protected:
			Param &atkParam, &dcyParam, &susParam, &rlsParam;
			Ruler ruler;
			std::vector<BoundsF> curve, curveMod;
			float atkV, dcyV, susV, rlsV, atkModV, dcyModV, susModV, rlsModV;

			void initRuler()
			{
				addAndMakeVisible(ruler);
				ruler.makeIncExpansionOfGF();
				ruler.setValToStrFunc([](float v)
				{
					return (v < 1000.f ? String(v) + "ms" : String(v * .001f) + "s");
				});
				ruler.setCID(CID::Interact);
				ruler.setDrawFirstVal(true);
			}

			// returns the lengthInMs of the envelope
			void updateCurve(std::vector<BoundsF>& c,
				float atkRatio, float dcyRatio, float sus, float rlsRatio,
				float pWidth) noexcept
			{
				const auto width = static_cast<float>(getWidth());
				const auto height = static_cast<float>(getHeight());

				const auto susHeight = height - sus * height;

				// sum and normalize ratios
				const auto ratioSum = atkRatio + dcyRatio + rlsRatio;
				if (ratioSum == 0.f)
				{
					c[0] = BoundsF(0.f, susHeight, width, 1.f);
					for (auto i = 1; i < getWidth(); ++i)
						c[i] = BoundsF(0.f, 0.f, 0.f, 0.f);
					return;
				}

				const auto ratioGain = 1.f / ratioSum;

				// normalized ratios
				const auto atkRatioNorm = atkRatio * ratioGain;
				const auto dcyRatioNorm = dcyRatio * ratioGain;
				const auto rlsRatioNorm = rlsRatio * ratioGain;

				// width of envelope states
				const auto atkWidth = atkRatioNorm * width;
				const auto dcyWidth = dcyRatioNorm * width;
				const auto rlsWidth = rlsRatioNorm * width;

				const auto atkWidthInv = atkWidth == 0.f ? 1.f : 1.f / atkWidth;
				const auto dcyWidthInv = dcyWidth == 0.f ? 1.f : 1.f / dcyWidth;
				const auto rlsWidthInv = rlsWidth == 0.f ? 1.f : 1.f / rlsWidth;

				//curve[0] = BoundsF(0.f, 0.f, atkWidth, height).reduced(utils.thicc);
				//curve[1] = BoundsF(atkWidth, 0.f, dcyWidth, height).reduced(utils.thicc);
				//curve[2] = BoundsF(atkWidth + dcyWidth, 0.f, rlsWidth, height).reduced(utils.thicc);

				auto state = kAttack;
				auto xOff = 0.f;
				auto x0 = 0.f, x1 = 1.f;
				auto y0 = height, y1 = height;

				for (auto i = 0; i < getWidth() - 1; ++i, ++x0, x1 = x0 + 1.f, y0 = y1)
				{
					switch (state)
					{
					case kAttack:
						y1 = height - x1 * atkWidthInv * height;
						if (y1 <= 0.f)
						{
							y1 = 0.f;
							state = kDecay;
							xOff = atkWidth;
							x0 = 0.f;
						}
						break;
					case kDecay:
						y1 = x1 * dcyWidthInv * susHeight;
						if (y1 >= susHeight)
						{
							y1 = susHeight;
							state = kRelease;
							xOff += dcyWidth;
							x0 = 0.f;
						}
						break;
					case kRelease:
						y1 = susHeight + x1 * rlsWidthInv * (height - susHeight);
						if (y1 >= height)
						{
							for (auto j = i; j < getWidth(); ++j, ++x0)
								c[j] = BoundsF(x0 + xOff, y1, pWidth, 1.f);
							return;
						}
						break;
					}

					auto yy = y0;
					auto h = y1 - y0;
					if (h < 0.f)
					{
						yy = y1;
						h = -h;
					}
					if (h < 1.f)
						h = 1.f;
					BoundsF bounds(x0 + xOff, yy, pWidth, h);
					c[i] = bounds;
				}
			}

			bool updateCurve() noexcept
			{
				// denormalized parameter values
				const auto atkModDenorm = atkParam.getValModDenorm();
				const auto dcyModDenorm = dcyParam.getValModDenorm();
				const auto rlsModDenorm = rlsParam.getValModDenorm();
				const auto susMod = susParam.getValMod();
				const auto atkDenorm = atkParam.getValueDenorm();
				const auto dcyDenorm = dcyParam.getValueDenorm();
				const auto rlsDenorm = rlsParam.getValueDenorm();
				const auto sus = susParam.getValue();

				// return if no change
				if (atkModV == atkModDenorm && dcyModV == dcyModDenorm && susModV == susMod && rlsModV == rlsModDenorm &&
					atkV == atkDenorm && dcyV == dcyDenorm && susV == sus && rlsV == rlsDenorm)
					return false;
				atkModV = atkModDenorm;
				dcyModV = dcyModDenorm;
				susModV = susMod;
				rlsModV = rlsModDenorm;
				atkV = atkDenorm;
				dcyV = dcyDenorm;
				susV = sus;
				rlsV = rlsDenorm;

				ruler.setLength(atkDenorm + dcyDenorm + rlsDenorm);

				// denormalized parameter end values
				const auto atkEndDenorm = atkParam.range.end;
				const auto dcyEndDenorm = dcyParam.range.end;
				const auto rlsEndDenorm = rlsParam.range.end;

				// ratio of denormalized values [0,1]
				const auto atkModRatio = atkModDenorm / atkEndDenorm;
				const auto dcyModRatio = dcyModDenorm / dcyEndDenorm;
				const auto rlsModRatio = rlsModDenorm / rlsEndDenorm;
				const auto atkRatio = atkDenorm / atkEndDenorm;
				const auto dcyRatio = dcyDenorm / dcyEndDenorm;
				const auto rlsRatio = rlsDenorm / rlsEndDenorm;

				updateCurve(curveMod, atkModRatio, dcyModRatio, susMod, rlsModRatio, 1.f);
				updateCurve(curve, atkRatio, dcyRatio, sus, rlsRatio, utils.thicc);
				return true;
			}
		};

		EnvelopeGeneratorMultiVoiceEditor(Utils& u, const String& title,
			PID atk, PID dcy, PID sus, PID rls) :
			Comp(u),
			labels{ Label(u), Label(u), Label(u), Label(u), Label(u) },
			envGenView(u, atk, dcy, sus, rls),
			knobs{ Knob(u), Knob(u), Knob(u), Knob(u) },
			modDials{ ModDial(u), ModDial(u), ModDial(u), ModDial(u) },
			adsrLabelsGroup()
		{
			layout.init
			(
				{ 1, 1, 1, 1 },
				{ 1, 5, 2, 1 }
			);

			for (auto& label : labels)
				addAndMakeVisible(label);
			addAndMakeVisible(envGenView);
			for (auto& knob : knobs)
				addAndMakeVisible(knob);
			for (auto& modDial : modDials)
				addAndMakeVisible(modDial);

			const auto fontKnobs = font::dosisBold();
			makeTextLabel(labels[kAttack], "A", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(labels[kDecay], "D", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(labels[kSustain], "S", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(labels[kRelease], "R", fontKnobs, Just::centred, CID::Txt);
			makeTextLabel(labels[kTitle], title, font::dosisMedium(), Just::centredLeft, CID::Txt);
			makeKnob(atk, knobs[kAttack]);
			makeKnob(dcy, knobs[kDecay]);
			makeKnob(sus, knobs[kSustain]);
			makeKnob(rls, knobs[kRelease]);
			modDials[kAttack].attach(atk);
			modDials[kDecay].attach(dcy);
			modDials[kSustain].attach(sus);
			modDials[kRelease].attach(rls);
			for(auto i = 0; i < kNumParameters; ++i)
				adsrLabelsGroup.add(labels[i]);
		}

		void paint(Graphics& g) override
		{
			const auto col1 = Colours::c(CID::Bg);
			const auto col2 = Colours::c(CID::Darken);
			const auto envGenBounds = envGenView.getBounds().toFloat();
			const auto envGenX = envGenBounds.getX();
			const auto envGenY = envGenBounds.getY();
			const auto envGenBtm = envGenBounds.getBottom();
			Gradient gradient(col1, envGenX, envGenY, col2, envGenX, envGenBtm, false);
			g.setGradientFill(gradient);
			g.fillRoundedRectangle(envGenBounds.withBottom(layout.getY(-1)), utils.thicc);
		}

		void resized() override
		{
			const auto thicc = utils.thicc;
			layout.resized(getLocalBounds().toFloat());
			layout.place(labels[kTitle], 0, 0, 4, 1);
			labels[kTitle].setMaxHeight(thicc);
			envGenView.setBounds(layout(0, 1, 4, 1).reduced(thicc).toNearestInt());
			for (auto i = 0; i < kNumParameters; ++i)
			{
				layout.place(knobs[i], i, 2, 1, 1);
				locateAtKnob(modDials[i], knobs[i]);
				layout.place(labels[i], i, 3, 1, 1);
			}
			adsrLabelsGroup.setMaxHeight();
		}

	protected:
		std::array<Label, kNumParameters + 1> labels;
		EnvGenView envGenView;
		std::array<Knob, kNumParameters> knobs;
		std::array<ModDial, kNumParameters> modDials;
		LabelGroup adsrLabelsGroup;
	};
}