#pragma once
#include "ModalFilterMaterialEditor.h"
#include "ModalParamsEditor.h"
#include "DropDownMenu.h"

namespace gui
{
	struct ModalModuleEditor :
		public Comp
	{
		ModalModuleEditor(Utils& u) :
			Comp(u),
			title(u),
			buttonA(u),
			buttonB(u),
			buttonSolo(u),
			buttonDropDown(u),
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
			dropDown(u)
		{
			layout.init
			(
				{ 1 },
				{ 1, 13, 8 }
			);

			addAndMakeVisible(title);
			addAndMakeVisible(buttonA);
			addAndMakeVisible(buttonB);
			addAndMakeVisible(buttonSolo);
			addAndMakeVisible(buttonDropDown);
			for(auto& m : materialEditors)
				addChildComponent(m);
			materialEditors[0].setVisible(true);
			addAndMakeVisible(params);
			addChildComponent(dropDown);

			initTitle();
			initButtonAB();
			initButtonSolo();
			initDropDown();
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::c(CID::Darken));
			const auto thicc = utils.thicc;
			g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(thicc), thicc);
		}

		void resized() override
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
				title.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
				title.setMaxHeight(thicc * 2.f);
				x += buttonWidth;
				buttonA.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
				x += buttonWidth;
				buttonB.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
				x += buttonWidth;
				buttonSolo.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
				x += buttonWidth;
				buttonDropDown.setBounds(BoundsF(x, y, buttonWidth, h).toNearestInt());
			}
			for (auto& m : materialEditors)
				layout.place(m, 0, 1, 1, 1);
			params.setBounds(layout.bottom().toNearestInt());
			layout.place(dropDown, 0, 1, 1, 1, thicc);
		}

		Label title;
		Button buttonA, buttonB, buttonSolo, buttonDropDown;
		std::array<ModalMaterialEditor, 2> materialEditors;
		ModalParamsEditor params;
		DropDownMenu dropDown;

	private:
		void initTitle()
		{
			auto titleFont = font::dosisRegular();
			makeTextLabel(title, "Modal Filter:", titleFont, Just::centred, CID::Txt, "This is the module of the modal filter.");
		}

		void initButtonAB()
		{
			makeTextButton(buttonA, "A", "Observe and edit Material A!", CID::Interact);
			buttonA.type = Button::Type::kToggle;
			buttonA.onPaint = makeButtonOnPaint(true, getColour(CID::Bg));
			buttonA.onClick = [&](const Mouse&)
			{
				materialEditors[0].setVisible(true);
				materialEditors[1].setVisible(false);
				buttonA.value = 1.f;
				buttonB.value = 0.f;
				buttonA.repaint();
				buttonB.repaint();
			};

			makeTextButton(buttonB, "B", "Observe and edit Material B!", CID::Interact);
			buttonB.type = Button::Type::kToggle;
			buttonB.onPaint = makeButtonOnPaint(false, getColour(CID::Bg));
			buttonB.onClick = [&](const Mouse&)
			{
				materialEditors[0].setVisible(false);
				materialEditors[1].setVisible(true);
				buttonA.value = 0.f;
				buttonB.value = 1.f;
				buttonA.repaint();
				buttonB.repaint();
			};

			buttonA.value = 1.f;
		}

		void initButtonSolo()
		{
			addAndMakeVisible(buttonSolo);
			makeTextButton(buttonSolo, "S", "Listen to partials in isolation with the solo button!", CID::Interact);
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

		void initDropDown()
		{
			// GEN: SINE
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Gen: Sine", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(buttonA.value > .5f ? 0 : 1);
					dsp::modal2::generateSine(material);
				}
			);

			// GEN: SAW
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Gen: Saw", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(buttonA.value > .5f ? 0 : 1);
					dsp::modal2::generateSaw(material);
				}
			);

			// GEN: SQUARE
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Gen: Square", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(buttonA.value > .5f ? 0 : 1);
					dsp::modal2::generateSquare(material);
				}
			);

			// Copy To Other Material
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Copy To Other Material", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					const auto thisIdx = buttonA.value > .5f ? 0 : 1;
					const auto& materialThis = modalFilter.getMaterial(thisIdx);
					auto& materialThat = modalFilter.getMaterial(1 - thisIdx);
					materialThat.peakInfos = materialThis.peakInfos;
					materialThat.reportUpdate();
				}
			);

			// Proc: Vertical Flip
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Proc: Vertical Flip", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					const auto numFilters = dsp::modal2::NumFilters;
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					const auto matIdx = buttonA.value > .5f ? 0 : 1;
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
				}
			);

			// Proc: Horizontal Flip
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Proc: Horizontal Flip", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					const auto numFilters = dsp::modal2::NumFilters;
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					const auto matIdx = buttonA.value > .5f ? 0 : 1;
					auto& material = modalFilter.getMaterial(matIdx);
					auto& peaks = material.peakInfos;
					for (auto i = 0; i < numFilters; ++i)
					{
						auto& peak = peaks[i];
						peak.mag = 1.f - peak.mag;
					}
					material.reportUpdate();
				}
			);

			// Record Input
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Record Input", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					auto& processor = utils.audioProcessor.pluginProcessor;
					processor.recording.store(buttonA.value > .5f ? 0 : 1);
					processor.recSampleIndex = 0;
				}
			);

			// Rescure Overlaps
			dropDown.add
			(
				[](Graphics& g, const Button& btn)
				{
					setCol(g, CID::Interact);
					g.drawFittedText("Rescue Overlaps", btn.getLocalBounds(), Just::centred, 1);
				},
				[&](const Mouse&)
				{
					auto& modalFilter = utils.audioProcessor.pluginProcessor.modalFilter;
					auto& material = modalFilter.getMaterial(buttonA.value > .5f ? 0 : 1);
					auto& peakInfos = material.peakInfos;
					const auto numFilters = dsp::modal2::NumFilters;
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
									if (r0 == r1)
									{
										duplicate = { i, j };
										duplicateFound = true;
										break;
									}
								}

							}
						}
						if (!duplicateFound)
							return;

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

						material.reportUpdate();
					}
				}
			);

			dropDown.init();

			makeTextButton(buttonDropDown, "V", "Here you can find additional modal material features.", CID::Interact);
			buttonDropDown.onPaint = makeButtonOnPaint(true, getColour(CID::Bg));
			buttonDropDown.onClick = [&m = dropDown](const Mouse&)
			{
				m.setVisible(!m.isVisible());
			};
		}
	};
}