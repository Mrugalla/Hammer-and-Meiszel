#include "ModalModuleEditor.h"

namespace gui
{
	ModalModuleEditor::ModalModuleEditor(Utils& u) :
		Comp(u),
		buttonAB(u),
		buttonRatioFreqs(u),
		buttonSolo(u),
		buttonDropDownGens(u),
		buttonDropDownMisc(u),
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
		params(u),
		dropDownGens(u),
		dropDownMisc(u),
		updateMaterialFunc([](){}),
		wannaUpdate(-1)
	{
		layout.init
		(
			{ 1 },
			{ 1, 13, 8 }
		);

		addAndMakeVisible(buttonAB);
		addAndMakeVisible(buttonRatioFreqs);
		addAndMakeVisible(buttonSolo);
		addAndMakeVisible(buttonDropDownGens);
		addAndMakeVisible(buttonDropDownMisc);
		for (auto& m : materialEditors)
			addChildComponent(m);
		materialEditors[0].setVisible(true);
		addAndMakeVisible(params);
		addChildComponent(dropDownGens);
		addChildComponent(dropDownMisc);

		initButtonAB();
		initButtonRatioFreqs();
		initButtonSolo();
		initDropDown();

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
			const auto buttonWidth = w / 5.f;
			buttonAB.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonRatioFreqs.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonSolo.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonDropDownGens.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			x += buttonWidth;
			buttonDropDownMisc.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
		}
		for (auto& m : materialEditors)
			layout.place(m, 0, 1, 1, 1);
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
		};
	}

	void ModalModuleEditor::initButtonRatioFreqs()
	{
		makeTextButton(buttonRatioFreqs, "Ratios", "Toggle between displaying the ratio or frequencies of the modal material.", CID::Interact);
		buttonRatioFreqs.value = 1.f;
		buttonRatioFreqs.type = Button::Type::kToggle;
		buttonRatioFreqs.onClick = [&](const Mouse&)
		{
			buttonRatioFreqs.value = buttonRatioFreqs.value > .5f ? 0.f : 1.f;
			const auto e = buttonRatioFreqs.value > .5f;
			buttonRatioFreqs.label.setText(e ? "Ratios" : "Freqs");
			buttonRatioFreqs.repaint();
			for (auto& mv : materialEditors)
				mv.setShowRatios(e);
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
					const auto numFilters = dsp::modal::NumFilters;
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(matIdx);
					auto& peaks = material.peakInfos;

					auto maxRatio = peaks[0].ratio;
					for (auto i = 1; i < numFilters; ++i)
						maxRatio = std::max(maxRatio, peaks[i].ratio);

					const auto peaksCopy = peaks;
					for (auto i = 0; i < numFilters; ++i)
					{
						const auto j = numFilters - i - 1;
						const auto& peakJ = peaksCopy[j];
						auto& peak = peaks[i];
						peak.ratio = maxRatio - peakJ.ratio + 1.f;
						peak.mag = peakJ.mag;
						peak.freqHz = peakJ.freqHz;
					}

					material.updateKeytrackValues();
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
					const auto numFilters = dsp::modal::NumFilters;
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(matIdx);
					auto& peaks = material.peakInfos;
					for (auto i = 0; i < numFilters; ++i)
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
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, matIdx]()
				{
						const auto numFilters = dsp::modal::NumFilters;
						auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
						auto& material = modalFilter.getMaterial(matIdx);
						auto& peaks = material.peakInfos;
						Random rand;
						for (auto i = 0; i < numFilters; ++i)
						{
							auto& peak = peaks[i];
							peak.mag = rand.nextFloat();
						}
						material.updatePeakInfosFromGUI();
				};
				wannaUpdate = matIdx;
			},
			"Randomize Magnitudes", "Randomizes all of the modal material's magnitudes."
		);

		// Proc: Randomize Frequency Ratios
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				updateMaterialFunc = [&, matIdx]()
				{
					const auto numFilters = dsp::modal::NumFilters;
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(matIdx);
					auto& peaks = material.peakInfos;
					Random rand;
					for (auto i = 0; i < numFilters; ++i)
					{
						auto& peak = peaks[i];
						peak.ratio = 1.f * rand.nextFloat() * 16.f;
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
					const auto numFilters = dsp::modal::NumFilters;
					while (true)
					{
						Point duplicate;
						bool duplicateFound = false;
						for (auto i = 0; i < numFilters; ++i)
						{
							auto r0 = peakInfos[i].ratio;
							for (auto j = i + 1; j < numFilters; ++j)
							{
								if (i != j)
								{
									const auto r1 = peakInfos[j].ratio;
									const auto dif = std::abs(r1 - r0);
									const auto threshold = 0.001f;
									if (dif < threshold)
									{
										duplicate = { i, j };
										duplicateFound = true;
										break;
									}
								}

							}
						}
						if (!duplicateFound)
							break;

						const auto dIdx1 = duplicate.y;
						const auto d1Mag = peakInfos[dIdx1].mag;

						for (auto i = dIdx1; i < numFilters - 1; ++i)
						{
							peakInfos[i].mag = peakInfos[i + 1].mag;
							peakInfos[i].ratio = peakInfos[i + 1].ratio;
						}

						peakInfos[numFilters - 1].mag = d1Mag;

						const auto maxRatio = peakInfos[numFilters - 1].ratio;
						peakInfos[numFilters - 1].ratio = maxRatio + 1.f;
					}
					material.reportUpdate();
				};
				wannaUpdate = matIdx;
			},
			"Rescue Overlaps", "Puts overlapping partials somewhere else."
		);

		dropDownGens.init();
		dropDownMisc.init();
		buttonDropDownGens.init(dropDownGens, "Generators", "Generate modal materials from magic! (math)");
		buttonDropDownMisc.init(dropDownMisc, "Misc", "Here you can find additional modal material features.");
	}
}