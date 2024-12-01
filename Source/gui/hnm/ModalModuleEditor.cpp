#include "ModalModuleEditor.h"

namespace gui
{
	ModalModuleEditor::ModalModuleEditor(Utils& u) :
		Comp(u),
		buttonAB(u),
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
		dropDownMisc(u)
	{
		layout.init
		(
			{ 1 },
			{ 1, 13, 8 }
		);

		addAndMakeVisible(buttonAB);
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
		initButtonSolo();
		initDropDown();
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
			const auto buttonWidth = w * .25f;
			buttonAB.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
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
		buttonAB.value = 0.f;
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
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(buttonAB.value > .5f ? 1 : 0);
				dsp::modal::generateSine(material);
			},
			"Gen: Sine", "Generates a modal material with a single partial."
		);

		// GEN: SAW
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(buttonAB.value > .5f ? 1 : 0);
				dsp::modal::generateSaw(material);
			},
			"Gen: Saw", "Generates a sawtooth-shaped modal material."
		);

		// GEN: SQUARE
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(buttonAB.value > .5f ? 1 : 0);
				dsp::modal::generateSquare(material);
			},
			"Gen: Square", "Generates a square-shaped modal material."
		);

		// GEN: FIBONACCI
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(buttonAB.value > .5f ? 1 : 0);
				dsp::modal::generateFibonacci(material);
			},
			"Gen: Fibonacci", "Generates a modal material with Fibonacci ratios."
		);

		// GEN: PRIME
		dropDownGens.add
		(
			[&](const Mouse&)
			{
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(buttonAB.value > .5f ? 1 : 0);
				dsp::modal::generatePrime(material);
			},
			"Gen: Prime", "Generates a modal material with prime ratios."
		);

		// Copy To Other Material
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				const auto thisIdx = buttonAB.value > .5f ? 1 : 0;
				const auto& materialThis = modalFilter.getMaterial(thisIdx);
				auto& materialThat = modalFilter.getMaterial(1 - thisIdx);
				materialThat.peakInfos = materialThis.peakInfos;
				materialThat.reportUpdate();
			},
			"Copy To Other Material", "Copies the selected modal material to the other material."
		);

		// Proc: Vertical Flip
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto numFilters = dsp::modal::NumFilters;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
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
			},
			"Proc: Vertical Flip", "Flips the modal material's partials vertically."
		);

		// Proc: Horizontal Flip
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				const auto numFilters = dsp::modal::NumFilters;
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				const auto matIdx = buttonAB.value > .5f ? 1 : 0;
				auto& material = modalFilter.getMaterial(matIdx);
				auto& peaks = material.peakInfos;
				for (auto i = 0; i < numFilters; ++i)
				{
					auto& peak = peaks[i];
					peak.mag = 1.f - peak.mag;
				}
				material.reportUpdate();
			},
			"Proc: Horizontal Flip", "Flips the modal material's partials horizontally."
		);

		// Record Input
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				auto& processor = utils.audioProcessor.pluginProcessor;
				processor.recording.store(buttonAB.value > .5f ? 1 : 0);
				processor.recSampleIndex = 0;
			},
			"Record Input", "Records the input signal for modal analysis."
		);

		// Rescue Overlaps
		dropDownMisc.add
		(
			[&](const Mouse&)
			{
				auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
				auto& material = modalFilter.getMaterial(buttonAB.value > .5f ? 1 : 0);
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
			},
			"Rescue Overlaps", "Puts overlapping partials somewhere else."
		);

		dropDownGens.init();
		dropDownMisc.init();
		buttonDropDownGens.init(dropDownGens, "Generators", "Generate modal materials from magic! (math)");
		buttonDropDownMisc.init(dropDownMisc, "Misc", "Here you can find additional modal material features.");
	}
}