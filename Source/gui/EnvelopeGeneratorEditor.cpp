#include "EnvelopeGeneratorEditor.h"

namespace gui
{
	//EnvGenView

	EnvelopeGeneratorMultiVoiceEditor::EnvGenView::EnvGenView(Utils& u, PID susPID) :
		Comp(u),
		susParam(u.getParam(susPID)),
		atkParam(nullptr), dcyParam(nullptr), rlsParam(nullptr),
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

		add(Callback([&]()
		{
			if (updateCurve())
				repaint();
		}, 0, cbFPS::k30, true));
	}

	void EnvelopeGeneratorMultiVoiceEditor::EnvGenView::init(PID atkPID, PID dcyPID, PID rlsPID, bool isTemposync)
	{
		atkParam = utils.audioProcessor.params[atkPID];
		dcyParam = utils.audioProcessor.params[dcyPID];
		rlsParam = utils.audioProcessor.params[rlsPID];
		initRuler(isTemposync);
	}

	void EnvelopeGeneratorMultiVoiceEditor::EnvGenView::resized()
	{
		layout.resized(getLocalBounds().toFloat());
		layout.place(ruler, 0, 0, 1, 1);
		updateCurve();
	}

	void EnvelopeGeneratorMultiVoiceEditor::EnvGenView::paint(Graphics& g)
	{
		const Stroke stroke(utils.thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
		setCol(g, CID::Mod);
		g.strokePath(curveMod, stroke);
		setCol(g, CID::Interact);
		g.strokePath(curve, stroke);
	}

	void EnvelopeGeneratorMultiVoiceEditor::EnvGenView::initRuler(bool isTemposync)
	{
		addAndMakeVisible(ruler);
		ruler.setCID(CID::Hover);
		
		if (isTemposync)
		{
			ruler.setDrawFirstVal(false);
			ruler.setGetIncFunc([](float v)
			{
				auto measure = 1.f / 32.f;
				while (true)
				{
					if (v <= measure)
						return measure / 4.f;
					measure *= 2.f;
				}
			});
			ruler.setValToStrFunc([](float v)
			{
				if (v < 1.f)
				{
					auto vInv = 1.f / v;
					if (vInv == std::round(vInv))
						return "1 / " + String(vInv);
					vInv *= 3.f;
					return "3 / " + String(vInv);
				}
				else
				{
					return String(v) + " bar";
				}
			});
			return;
		}
		ruler.setDrawFirstVal(true);
		ruler.makeIncExpansionOfGF();
		ruler.setValToStrFunc([](float v)
		{
			return (v < 1000.f ? String(v) + "ms" : String(v * .001f) + "s");
		});
	}

	void EnvelopeGeneratorMultiVoiceEditor::EnvGenView::updateCurve(Path& c, float atkRatio,
		float dcyRatio, float sus, float rlsRatio) noexcept
	{
		const auto width = static_cast<float>(getWidth());
		const auto height = static_cast<float>(getHeight());

		const auto susHeight = height - sus * height;

		c.clear();

		// sum and normalize ratios
		const auto ratioSum = atkRatio + dcyRatio + rlsRatio;
		const bool onlySustain = ratioSum == 0.f;
		if (onlySustain)
		{
			c.startNewSubPath(0.f, susHeight);
			c.lineTo(width, susHeight);
			return;
		}

		const auto ratioGain = 1.f / ratioSum;

		// normalized ratios
		const auto atkRatioNorm = atkRatio * ratioGain;
		const auto dcyRatioNorm = dcyRatio * ratioGain;

		// width of envelope states
		const auto atkX = atkRatioNorm * width;
		const auto dcyX = dcyRatioNorm * width;
		const auto rlsX = atkX + dcyX;

		if (rlsX == 0.f)
			c.startNewSubPath(0.f, susHeight);
		else
		{
			if (atkX == 0.f)
				c.startNewSubPath(0.f, 0.f);
			else
			{
				c.startNewSubPath(0.f, height);
				c.quadraticTo(atkX * .5f, 0.f, atkX, 0.f);
			}

			if (dcyX == 0.f)
				c.lineTo(rlsX, susHeight);
			else
				c.quadraticTo(atkX + dcyX * .5f, susHeight, rlsX, susHeight);
		}

		if (rlsX == width)
			c.lineTo(width, height);
		else
		{
			const auto dist = width - rlsX;
			c.quadraticTo(rlsX + dist * .5f, height, width, height);
		}
	}

	bool EnvelopeGeneratorMultiVoiceEditor::EnvGenView::updateCurve() noexcept
	{
		if (atkParam == nullptr || dcyParam == nullptr || rlsParam == nullptr)
			return false;

		// denormalized parameter values
		const auto atkModDenorm = atkParam->getValModDenorm();
		const auto dcyModDenorm = dcyParam->getValModDenorm();
		const auto rlsModDenorm = rlsParam->getValModDenorm();
		const auto susMod = susParam.getValMod();
		const auto atkDenorm = atkParam->getValueDenorm();
		const auto dcyDenorm = dcyParam->getValueDenorm();
		const auto rlsDenorm = rlsParam->getValueDenorm();
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
		const auto atkEndDenorm = atkParam->range.end;
		const auto dcyEndDenorm = dcyParam->range.end;
		const auto rlsEndDenorm = rlsParam->range.end;

		// ratio of denormalized values [0,1]
		const auto atkModRatio = atkModDenorm / atkEndDenorm;
		const auto dcyModRatio = dcyModDenorm / dcyEndDenorm;
		const auto rlsModRatio = rlsModDenorm / rlsEndDenorm;
		const auto atkRatio = atkDenorm / atkEndDenorm;
		const auto dcyRatio = dcyDenorm / dcyEndDenorm;
		const auto rlsRatio = rlsDenorm / rlsEndDenorm;

		updateCurve(curveMod, atkModRatio, dcyModRatio, susMod, rlsModRatio);
		updateCurve(curve, atkRatio, dcyRatio, sus, rlsRatio);
		return true;
	}
	
	//EnvelopeGeneratorMultiVoiceEditor

	EnvelopeGeneratorMultiVoiceEditor::EnvelopeGeneratorMultiVoiceEditor(Utils& u, const String& title,
		PID atk, PID dcy, PID sus, PID rls, PIDsTemposync* temposyncPIDs) :
		Comp(u),
		labels{ Label(u), Label(u), Label(u), Label(u), Label(u) },
		envGenView(u, sus),
		knobs{ Knob(u), Knob(u), Knob(u), Knob(u) },
		modDials{ ModDial(u), ModDial(u), ModDial(u), ModDial(u) },
		adsrLabelsGroup(),
		temposync(u),
		buttonRandomizer(u, "randenv" + title),
		temposyncEnabled(false)
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
		addAndMakeVisible(buttonRandomizer);

		if (temposyncPIDs)
		{
			addAndMakeVisible(temposync);
			makeParameter(temposync, temposyncPIDs->temposync, Button::Type::kToggle, "Sync");

			const auto& temposyncParam = u.getParam(PID::EnvGenModTemposync);
			temposyncEnabled = temposyncParam.getValMod() > .5f;

			add(Callback([&, a = atk, d = dcy, r = rls,
				aSync = temposyncPIDs->atk,
				dSync = temposyncPIDs->dcy,
				rSync = temposyncPIDs->rls]()
				{
					const auto& temposyncParam = u.getParam(PID::EnvGenModTemposync);
					const auto enabled = temposyncParam.getValMod() > .5f;
					if (enabled == temposyncEnabled)
						return;
					temposyncEnabled = enabled;

					const auto aC = temposyncEnabled ? aSync : a;
					const auto dC = temposyncEnabled ? dSync : d;
					const auto rC = temposyncEnabled ? rSync : r;
					makeKnob(aC, knobs[kAttack]);
					makeKnob(dC, knobs[kDecay]);
					makeKnob(rC, knobs[kRelease]);
					modDials[kAttack].attach(aC);
					modDials[kDecay].attach(dC);
					modDials[kRelease].attach(rC);

					envGenView.init(aC, dC, rC, temposyncEnabled);
					repaint();
				}, kTSCheckCB, cbFPS::k15, true));
		}

		if(temposyncEnabled)
		{
			makeKnob(temposyncPIDs->atk, knobs[kAttack]);
			makeKnob(temposyncPIDs->dcy, knobs[kDecay]);
			makeKnob(temposyncPIDs->rls, knobs[kRelease]);
			modDials[kAttack].attach(temposyncPIDs->atk);
			modDials[kDecay].attach(temposyncPIDs->dcy);
			modDials[kRelease].attach(temposyncPIDs->rls);

			envGenView.init(temposyncPIDs->atk, temposyncPIDs->dcy, temposyncPIDs->rls, true);
		}
		else
		{
			makeKnob(atk, knobs[kAttack]);
			makeKnob(dcy, knobs[kDecay]);
			makeKnob(rls, knobs[kRelease]);
			modDials[kAttack].attach(atk);
			modDials[kDecay].attach(dcy);
			modDials[kRelease].attach(rls);

			envGenView.init(atk, dcy, rls, false);
		}

		makeKnob(sus, knobs[kSustain]);
		modDials[kSustain].attach(sus);

		const auto fontKnobs = font::dosisBold();
		makeTextLabel(labels[kAttack], "A", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(labels[kDecay], "D", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(labels[kSustain], "S", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(labels[kRelease], "R", fontKnobs, Just::centred, CID::Txt);
		makeTextLabel(labels[kTitle], title, font::dosisMedium(), Just::centredLeft, CID::Txt);
		for (auto i = 0; i < kNumParameters; ++i)
			adsrLabelsGroup.add(labels[i]);

		buttonRandomizer.add(atk);
		buttonRandomizer.add(dcy);
		buttonRandomizer.add(sus);
		buttonRandomizer.add(rls);
		if (temposyncPIDs)
		{
			buttonRandomizer.add(temposyncPIDs->temposync);
			buttonRandomizer.add(temposyncPIDs->atk);
			buttonRandomizer.add(temposyncPIDs->dcy);
			buttonRandomizer.add(temposyncPIDs->rls);
		}
	}

	void EnvelopeGeneratorMultiVoiceEditor::paint(Graphics& g)
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

	void EnvelopeGeneratorMultiVoiceEditor::resized()
	{
		const auto thicc = utils.thicc;
		layout.resized(getLocalBounds().toFloat());
		layout.place(labels[kTitle], 0, 0, 2, 1);
		labels[kTitle].setMaxHeight(thicc);
		envGenView.setBounds(layout(0, 1, 4, 1).reduced(thicc).toNearestInt());
		for (auto i = 0; i < kNumParameters; ++i)
		{
			auto& knob = knobs[i];
			layout.place(knob, i, 2, 1, 1);
			locateAtKnob(modDials[i], knob);
			layout.place(labels[i], i, 3, 1, 1);
		}
		adsrLabelsGroup.setMaxHeight();
		layout.place(buttonRandomizer, 3, 0, 1, 1);
		layout.place(temposync, 2, 0, 1, 1);
	}
}