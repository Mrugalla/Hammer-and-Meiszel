#include "ModalModuleEditor.h"

namespace gui
{
	ModalModuleEditor::ModalModuleEditor(Utils& u) :
		Comp(u),
		buttonAB(u),
		buttonSolo(u),
		buttonDropDownGens(u),
		buttonDropDownProcess(u),
		buttonRandomizer(u, "randmodal"),
		buttonFixed(u),
		materialEditors
		{
			ModalMaterialEditor
			(
				utils,
				utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(0),
				utils.audioProcessor.pluginProcessor.modalFilter.getActives()
			),
			ModalMaterialEditor
			(
				utils,
				utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(1),
				utils.audioProcessor.pluginProcessor.modalFilter.getActives()
			)
		},
		partialsFixedEditors
		{
			ModalPartialsFixedEditor(utils),
			ModalPartialsFixedEditor(utils)
		},
		params(u),
		dropDownGens(u),
		dropDownProcess(u),
		randSeedVertical(u.getProps(), "randvrtcl"),
		randSeedHorizontal(u.getProps(), "randhrzntl"),
		randSeedFixedPartials(u.getProps(), "randfixd")
	{
		layout.init
		(
			{ 1 },
			{ 1, 21, 13 }
		);

		addAndMakeVisible(buttonAB);
		addAndMakeVisible(buttonSolo);
		addAndMakeVisible(buttonDropDownGens);
		addAndMakeVisible(buttonDropDownProcess);
		addAndMakeVisible(buttonRandomizer);
		addAndMakeVisible(buttonFixed);
		for (auto& m : materialEditors)
			addChildComponent(m);
		for (auto& p : partialsFixedEditors)
			addChildComponent(p);
		materialEditors[0].setVisible(true);
		addAndMakeVisible(params);
		addChildComponent(dropDownGens);
		addChildComponent(dropDownProcess);

		initButtonAB();
		initButtonSolo();
		initDropDown();
		initRandomizer();

		buttonFixed.type = Button::Type::kToggle;
		makeTextButton(buttonFixed, "Formants", "Click here to adjust the selected material's formants!", CID::Interact);
		buttonFixed.onClick = [&](const Mouse&)
		{
			buttonFixed.value = 1.f - buttonFixed.value;
			const bool showFixed = buttonFixed.value > .5f;
			const auto matIdx = buttonAB.value > .5f ? 1 : 0;
			partialsFixedEditors[matIdx].setVisible(showFixed);
		};
		buttonFixed.value = 0.f;

		addEvt([&](const evt::Type t, const void*)
		{
			if (t == evt::Type::ClickedEmpty)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				partialsFixedEditors[matIdx].setVisible(false);
				buttonFixed.value = 0.f;
			}
		});
	}

	void ModalModuleEditor::paint(Graphics& g)
	{
		g.setColour(Colours::c(CID::Darken));
		const auto thicc = utils.thicc;
		g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(thicc), thicc);
	}

	void ModalModuleEditor::resized()
	{
		const auto thicc = utils.thicc;
		layout.resized(getLocalBounds().toFloat().reduced(thicc * 4.f));

		const auto top = layout.top();
		{
			auto x = 0.f;
			const auto y = top.getY();
			const auto w = top.getWidth();
			const auto h = top.getHeight();
			const auto buttonWidth = w / 6.f;
			buttonAB.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonSolo.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonDropDownGens.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonDropDownProcess.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonRandomizer.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonFixed.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
		}
		for (auto& m : materialEditors)
			layout.place(m, 0, 1, 1, 1);
		for (auto& p : partialsFixedEditors)
			layout.place(p, 0, 1.f, 1, .8f, true);
		params.setBounds(layout.bottom().toNearestInt());
		const auto dropDownsBounds = layout(0, 1, 1, 1, thicc).toNearestInt();
		dropDownGens.setBounds(dropDownsBounds);
		dropDownProcess.setBounds(dropDownsBounds);
	}

	void ModalModuleEditor::initButtonAB()
	{
		makeTextButton(buttonAB, "A/B: A", "Observe and edit Materials A/B! (Blend them with the blend parameter below)", CID::Interact);
		buttonAB.value = 0.f;
		buttonAB.type = Button::Type::kToggle;
		buttonAB.onClick = [&](const Mouse&)
		{
			const auto nVal = buttonAB.value > .5f ? 0.f : 1.f;
			buttonAB.value = nVal;
			const auto e = nVal > .5f;
			buttonAB.label.setText(e ? "A/B: B" : "A/B: A");
			buttonAB.repaint();
			materialEditors[0].setVisible(!e);
			materialEditors[1].setVisible(e);

			const auto fixedVisible = buttonFixed.value > .5f;
			if (fixedVisible)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				partialsFixedEditors[matIdx].setVisible(true);
				partialsFixedEditors[1 - matIdx].setVisible(false);
			}
		};
	}

	void ModalModuleEditor::initButtonSolo()
	{
		addAndMakeVisible(buttonSolo);
		makeTextButton(buttonSolo, "Solo", "Listen to partials in isolation with the solo button!", CID::Interact);
		buttonSolo.value = 0.f;
		buttonSolo.type = Button::Type::kToggle;
		buttonSolo.onClick = [&](const Mouse&)
		{
			buttonSolo.value = std::round(1.f - buttonSolo.value);
			utils.audioProcessor.pluginProcessor.modalFilter.setSolo(buttonSolo.value > .5f);
			for (auto& mv : materialEditors)
			{
				if (mv.isShowing())
				{
					mv.updateActives(buttonSolo.value > .5f);
					mv.repaint();
				}
			}
		};
	}

	void ModalModuleEditor::initDropDown()
	{
		const auto updateFunc = [&](int matIdx)
		{
			auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
			auto& material = modalFilter.getMaterial(matIdx);
			material.updatePeakInfosFromGUI();
		};

		// GEN: SINE
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(i);
				dsp::modal::generateSine(material);
			},
			"Sine", "Generates a modal material with a single partial."
		);

		// GEN: SAW
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(i);
				dsp::modal::generateSaw(material);
			},
			"Saw", "Generates a sawtooth-shaped modal material."
		);

		// GEN: SQUARE
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(i);
				dsp::modal::generateSquare(material);
			},
			"Square", "Generates a square-shaped modal material."
		);

		// GEN: FIBONACCI
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(i);
				dsp::modal::generateFibonacci(material);
			},
			"Fibonacci", "Generates a modal material with Fibonacci ratios."
		);

		// GEN: PRIME
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(i);
				dsp::modal::generatePrime(material);
			},
			"Prime", "Generates a modal material with prime ratios."
		);

		const auto randRatiosFunc = [&](const Mouse& mouse, int matIdx)
		{
			randSeedHorizontal.updateSeed(mouse.mods.isLeftButtonDown());
			const auto numPartials = dsp::modal::NumPartials;
			auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
			auto& material = modalFilter.getMaterial(matIdx);
			auto& peaks = material.peakInfos;
			for (auto i = 1; i < numPartials; ++i)
			{
				auto& peak = peaks[i];
				peak.fc = 1.f * randSeedHorizontal() * 32.f;
			}
		};

		const auto randMagsFunc = [&](const Mouse& mouse, int matIdx)
		{
			randSeedVertical.updateSeed(mouse.mods.isLeftButtonDown());
			const auto numPartials = dsp::modal::NumPartials;
			auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
			auto& material = modalFilter.getMaterial(matIdx);
			auto& peaks = material.peakInfos;
			for (auto i = 0; i < numPartials; ++i)
			{
				auto& peak = peaks[i];
				peak.mag = randSeedVertical();
			}
		};

		// GEN: Randomize All
		dropDownGens.add
		(
			[&, a = randRatiosFunc, b = randMagsFunc, u = updateFunc](const Mouse& mouse)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				a(mouse, matIdx);
				b(mouse, matIdx); 
				u(matIdx);
			},
			"Rand: All", "Randomizes every property of the modal material."
		);

		// GEN: Randomize Frequency Ratios
		dropDownGens.add
		(
			[&, a = randRatiosFunc, u = updateFunc](const Mouse& mouse)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				a(mouse, matIdx);
				u(matIdx);
			},
			"Rand: Ratios", "Randomizes the modal material's ratios."
		);

		// GEN: Randomize Magnitudes
		dropDownGens.add
		(
			[&, b = randMagsFunc, u = updateFunc](const Mouse& mouse)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				b(mouse, matIdx);
				u(matIdx);
			},
			"Rand: Magnitudes", "Randomizes the modal material's magnitudes."
		);

		// GEN: Record Input
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& processor = utils.audioProcessor.pluginProcessor;
				processor.recording.store(i);
				processor.recSampleIndex = 0;
			},
			"Rec Input", "Records the input signal for modal analysis."
		);

		// Proc: Copy To Other Material
		dropDownProcess.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				const auto& materialThis = modalFilter.getMaterial(i);
				auto& materialThat = modalFilter.getMaterial(1 - i);
				materialThat.peakInfos = materialThis.peakInfos;
				materialThat.reportUpdate();
			},
			"Copy To Other Material", "Copies the selected modal material to the other material."
		);

		// Proc: Vertical Flip
		dropDownProcess.add
		(
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				const auto numPartials = dsp::modal::NumPartials;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(matIdx);
				auto& peaks = material.peakInfos;

				auto maxRatio = peaks[0].fc;
				for (auto i = 1; i < numPartials; ++i)
					maxRatio = std::max(maxRatio, peaks[i].fc);

				const auto peaksCopy = peaks;
				for (auto i = 0; i < numPartials; ++i)
				{
					const auto j = numPartials - i - 1;
					const auto& peakJ = peaksCopy[j];
					auto& peak = peaks[i];
					peak.fc = maxRatio - peakJ.fc + 1.f;
					peak.mag = peakJ.mag;
				}
				material.reportUpdate();
			},
			"Vertical Flip", "Flips the modal material's partials vertically."
		);

		// Proc: Horizontal Flip
		dropDownProcess.add
		(
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				const auto numPartials = dsp::modal::NumPartials;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(matIdx);
				auto& peaks = material.peakInfos;
				for (auto i = 0; i < numPartials; ++i)
				{
					auto& peak = peaks[i];
					peak.mag = 1.f - peak.mag;
				}
				material.reportUpdate();
			},
			"Horizontal Flip", "Flips the modal material's partials horizontally."
		);

		// Rescue Overlaps
		dropDownProcess.add
		(
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(matIdx);
				auto& peakInfos = material.peakInfos;
				const auto numPartials = dsp::modal::NumPartials;

				std::vector<int> duplicateIndexes;
				size_t numDuplicates = 0;
				do
				{
					for (auto i = 0; i < numDuplicates; ++i)
					{
						const auto dIdx = duplicateIndexes[i];
						++peakInfos[dIdx].fc;
					}

					duplicateIndexes.clear();
					for (auto i = 0; i < numPartials; ++i)
					{
						const PointD r0(peakInfos[i].fc, peakInfos[i].mag);
						for (auto j = i + 1; j < numPartials; ++j)
							if (i != j)
							{
								const PointD r1(peakInfos[j].fc, peakInfos[j].mag);
								const LineD line(r0, r1);
								const auto dif = line.getLengthSquared();
								const auto threshold = 0.01;
								if (dif < threshold)
								{
									bool alreadyFound = false;
									for (const auto& dIdx : duplicateIndexes)
										if (dIdx == i)
										{
											alreadyFound = true;
											break;
										}
									if (!alreadyFound)
										duplicateIndexes.push_back(j);
								}
							}
					}

					numDuplicates = duplicateIndexes.size();
				} while (numDuplicates != 0);

				material.reportUpdate();
			},
			"Rescue Overlaps", "This button puts overlapping partials somewhere else."
		);

		dropDownGens.init();
		dropDownProcess.init();
		buttonDropDownGens.init(dropDownGens, "Generate", "Generate modal materials from magic! (math)");
		buttonDropDownProcess.init(dropDownProcess, "Process", "Process the modal material in various ways!");
	}

	void ModalModuleEditor::initRandomizer()
	{
		buttonRandomizer.add(PID::CombFeedback);
		buttonRandomizer.add(PID::CombFeedbackEnv);
		buttonRandomizer.add(PID::CombFeedbackWidth);
		buttonRandomizer.add(PID::CombUnison);
		buttonRandomizer.add(PID::ModalBlend);
		buttonRandomizer.add(PID::ModalBlendEnv);
		buttonRandomizer.add(PID::ModalBlendBreite);
		buttonRandomizer.add(PID::ModalHarmonie);
		buttonRandomizer.add(PID::ModalHarmonieEnv);
		buttonRandomizer.add(PID::ModalHarmonieBreite);
		buttonRandomizer.add(PID::ModalKraft);
		buttonRandomizer.add(PID::ModalKraftEnv);
		buttonRandomizer.add(PID::ModalKraftBreite);
		buttonRandomizer.add(PID::Damp);
		buttonRandomizer.add(PID::DampEnv);
		buttonRandomizer.add(PID::DampWidth);
		buttonRandomizer.add(PID::ModalResonanz);
		buttonRandomizer.add(PID::ModalResonanzEnv);
		buttonRandomizer.add(PID::ModalResonanzBreite);

		const auto randModalFunc = [&](ButtonRandomizer::RandomSeed& rand)
		{
			auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
			for (auto i = 0; i < 2; ++i)
				modalFilter.randomizeMaterial(rand, i);
		};

		buttonRandomizer.add(randModalFunc);
	}
}