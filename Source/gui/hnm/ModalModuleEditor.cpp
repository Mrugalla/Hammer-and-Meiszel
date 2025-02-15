#include "ModalModuleEditor.h"

namespace gui
{
	ModalModuleEditor::ModalModuleEditor(Utils& u) :
		Comp(u),
		buttonAB(u),
		buttonSolo(u),
		buttonDropDownGens(u),
		buttonDropDownMisc(u),
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
		dropDownMisc(u),
		updateMaterialFunc([](){}),
		randSeedVertical(u.getProps(), "randvrtcl"),
		randSeedHorizontal(u.getProps(), "randhrzntl"),
		randSeedFixedPartials(u.getProps(), "randfixd"),
		wannaUpdate(-1)
	{
		layout.init
		(
			{ 1 },
			{ 1, 21, 13 }
		);

		addAndMakeVisible(buttonAB);
		addAndMakeVisible(buttonSolo);
		addAndMakeVisible(buttonDropDownGens);
		addAndMakeVisible(buttonDropDownMisc);
		addAndMakeVisible(buttonRandomizer);
		addAndMakeVisible(buttonFixed);
		for (auto& m : materialEditors)
			addChildComponent(m);
		for (auto& p : partialsFixedEditors)
			addChildComponent(p);
		materialEditors[0].setVisible(true);
		addAndMakeVisible(params);
		addChildComponent(dropDownGens);
		addChildComponent(dropDownMisc);

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

		add(Callback([&]()
		{
			if (wannaUpdate == -1)
				return;
			const auto& mat = utils.audioProcessor.pluginProcessor.modalFilter.getMaterial(wannaUpdate);
			const auto status = mat.status.load();
			if (status != dsp::modal::StatusMat::Processing)
				return;
			updateMaterialFunc();
			wannaUpdate = -1;
		}, 0, cbFPS::k15, true));
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
			buttonDropDownMisc.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonRandomizer.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonFixed.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
		}
		for (auto& m : materialEditors)
			layout.place(m, 0, 1, 1, 1);
		for (auto& p : partialsFixedEditors)
			layout.place(p, 0, 1, 1, 1, true);
		params.setBounds(layout.bottom().toNearestInt());
		const auto dropDownsBounds = layout(0, 1, 1, 1, thicc).toNearestInt();
		dropDownGens.setBounds(dropDownsBounds);
		dropDownMisc.setBounds(dropDownsBounds);
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
		// GEN: SINE
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, i]()
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(i);
					dsp::modal::generateSine(material);
				};
				wannaUpdate = i;
			},
			"Gen: Sine", "Generates a modal material with a single partial."
		);

		// GEN: SAW
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, i]()
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(i);
					dsp::modal::generateSaw(material);
				};
				wannaUpdate = i;
			},
			"Gen: Saw", "Generates a sawtooth-shaped modal material."
		);

		// GEN: SQUARE
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, i]()
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(i);
					dsp::modal::generateSquare(material);
				};
				wannaUpdate = i;
			},
			"Gen: Square", "Generates a square-shaped modal material."
		);

		// GEN: FIBONACCI
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, i]()
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(i);
					dsp::modal::generateFibonacci(material);
				};
				wannaUpdate = i;
			},
			"Gen: Fibonacci", "Generates a modal material with Fibonacci ratios."
		);

		// GEN: PRIME
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, i]()
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(i);
					dsp::modal::generatePrime(material);
				};
				wannaUpdate = i;
			},
			"Gen: Prime", "Generates a modal material with prime ratios."
		);

		// Copy To Other Material
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, i]()
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					const auto& materialThis = modalFilter.getMaterial(i);
					auto& materialThat = modalFilter.getMaterial(1 - i);
					materialThat.peakInfos = materialThis.peakInfos;
					materialThat.reportUpdate();
				};
				wannaUpdate = 1 - i;
			},
			"Copy To Other Material", "Copies the selected modal material to the other material."
		);

		// Proc: Vertical Flip
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, matIdx]()
				{
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
				};
				wannaUpdate = matIdx;
			},
			"Proc: Vertical Flip", "Flips the modal material's partials vertically."
		);

		// Proc: Horizontal Flip
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, matIdx]()
				{
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
				};
				wannaUpdate = matIdx;
			},
			"Proc: Horizontal Flip", "Flips the modal material's partials horizontally."
		);

		// Proc: Randomize Magnitudes
		dropDownMisc.add
		(
			[&](const Mouse& mouse)
			{
				randSeedVertical.updateSeed(mouse.mods.isLeftButtonDown());

				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, matIdx]()
				{
						const auto numPartials = dsp::modal::NumPartials;
						auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
						auto& material = modalFilter.getMaterial(matIdx);
						auto& peaks = material.peakInfos;
						for (auto i = 0; i < numPartials; ++i)
						{
							auto& peak = peaks[i];
							peak.mag = randSeedVertical();
						}
						material.updatePeakInfosFromGUI();
				};
				wannaUpdate = matIdx;
			},
			"Randomize Magnitudes", "Randomizes all of the partials' magnitudes."
		);

		// Proc: Randomize Frequency Ratios
		dropDownMisc.add
		(
			[&](const Mouse& mouse)
			{
				randSeedHorizontal.updateSeed(mouse.mods.isLeftButtonDown());

				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, matIdx]()
				{
					const auto numPartials = dsp::modal::NumPartials;
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(matIdx);
					auto& peaks = material.peakInfos;
					for (auto i = 1; i < numPartials; ++i)
					{
						auto& peak = peaks[i];
						peak.fc = 1.f * randSeedHorizontal() * 32.f;
					}
					material.updatePeakInfosFromGUI();
				};
				wannaUpdate = matIdx;
				
			},
			"Randomize Ratios", "Randomizes all of the modal material's ratios."
		);

		// Record Input
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto i = buttonAB.value > .5f ? 1 : 0;
				auto& processor = utils.audioProcessor.pluginProcessor;
				processor.recording.store(i);
				processor.recSampleIndex = 0;
			},
			"Record Input", "Records the input signal for modal analysis."
		);

		// Rescue Overlaps
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, matIdx]()
				{
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
				};
				wannaUpdate = matIdx;
			},
			"Rescue Overlaps", "This button puts overlapping partials somewhere else."
		);

		dropDownGens.init();
		dropDownMisc.init();
		buttonDropDownGens.init(dropDownGens, "Generators", "Generate modal materials from magic! (math)");
		buttonDropDownMisc.init(dropDownMisc, "Misc", "Here you can find additional modal material features.");
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
		buttonRandomizer.add(PID::ModalResoDamp);
		buttonRandomizer.add(PID::ModalResoDampEnv);
		buttonRandomizer.add(PID::ModalResoDampBreite);
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