#pragma once
#include "Label.h"
#include "../audio/dsp/modal/Material.h"

namespace gui
{
	struct ModalMaterialView :
		public Comp,
		public FileDragAndDropTarget
	{
		static constexpr float Sensitive = .1f;

		using Status = dsp::modal::Status;
		using Material = dsp::modal::Material;
		using PeakInfo = Material::PeakInfo;
		static constexpr int NumFilters = dsp::modal::NumFilters;
		
		struct Partial
		{
			Partial() :
				x(0.f), y(0.f)
			{
			}

			float x, y;
		};

		using Partials = std::array<Partial, NumFilters>;

		struct DragAnimationComp :
			public Comp
		{
			DragAnimationComp(Utils& u) :
				Comp(u),
				isInterested(false),
				error("")
			{
				setInterceptsMouseClicks(false, false);

				const auto fps = cbFPS::k30;
				add(Callback([&, speed = msToInc(5000.f, fps)]()
				{
					if (!isVisible())
						return;
					auto& phase = callbacks[0].phase;
					phase += speed;
					if (phase >= 1.f)
						--phase;
					repaint();
				}, 0, fps, false));
			}

			void start()
			{
				setVisible(true);
				callbacks[0].start(0.f);
			}

			void stop()
			{
				setVisible(false);
				callbacks[0].stop(0.f);
			}

			void paint(Graphics& g) override
			{
				const auto w = static_cast<float>(getWidth());
				const auto h = static_cast<float>(getHeight());
				const auto maxDimen = std::max(w, h);
				const Colour white(0xffffffff);
				const auto whiteTrans = white.withAlpha(0.4f);
				g.setColour(whiteTrans);
				const auto numLines = static_cast<int>(maxDimen / utils.thicc * .2f);
				const auto numLinesF = static_cast<float>(numLines);
				const auto numLinesInv = 1.f / numLinesF;
				for (auto i = 0; i < numLines; ++i)
				{
					const auto iF = static_cast<float>(i);
					auto iInv = iF * numLinesInv + callbacks[0].phase;
					if (iInv >= 1.f)
						--iInv;
					PointF midPoint(iInv * w, iInv * h);
					const auto line = LineF::fromStartAndAngle(midPoint, maxDimen, PiQuart).withLengthenedStart(maxDimen);
					g.drawLine(line, utils.thicc);
				}

				if (isInterested)
				{
					g.setColour(juce::Colours::green);
					g.drawFittedText("Yes, pls drop this!! :>", getLocalBounds(), Just::centred, 1);
				}
				else
				{
					g.setColour(juce::Colours::red);
					g.drawFittedText("Oh no, there was an error :<\n\n" + error, getLocalBounds(), Just::centred, 1);
				}
			}

			bool isInterested;
			String error;
		};

		struct Draggerfall
		{
			Draggerfall() :
				width(1.f),
				height(1.f),
				xRel(1.f),
				xAbs(1.f),
				lenRel(.1f),
				lenAbs(2.f),
				lenAbsHalf(1.f),
				selection()
			{
				for (auto& s : selection)
					s = false;
			}

			void resized(Partials& partials, float w, float h) noexcept
			{
				width = w;
				height = h;
				updateX(partials, xAbs, false);
				lenAbs = juce::jlimit(1.f, width, (lenRel + 0.f) * width);
				lenRel = lenAbs / width;
				lenAbsHalf = lenAbs * .5f;
			}

			void updateX(Partials& partials, float x, bool doUpdateSelection) noexcept
			{
				xAbs = x;
				xRel = xAbs / width;
				if(doUpdateSelection)
					updateSelection(partials);
			}

			void addLength(Partials& partials, float valRel) noexcept
			{
				lenAbs = juce::jlimit(1.f, width, (lenRel + valRel) * width);
				lenRel = lenAbs / width;
				lenAbsHalf = lenAbs * .5f;
				updateSelection(partials);
			}

			void paint(Graphics& g)
			{
				g.setColour(Colours::c(CID::Hover));
				auto x = xAbs - lenAbsHalf;
				auto w = lenAbs;
				g.fillRect(x, 0.f, w, height);
			}

			bool isSelected(int i) const noexcept
			{
				return selection[i];
			}

			void clearSelection() noexcept
			{
				for (auto& s : selection)
					s = false;
			}

			bool selectionEmpty() const noexcept
			{
				for (auto s : selection)
					if (s)
						return false;
				return true;
			}

		protected:
			float width, height, xRel, xAbs, lenRel, lenAbs, lenAbsHalf;
			std::array<bool, NumFilters> selection;

			void updateSelection(Partials& partials) noexcept
			{
				const auto lowerLimit = xAbs - lenAbsHalf;
				const auto upperLimit = lowerLimit + lenAbs;
				for (auto i = 0; i < NumFilters; ++i)
				{
					const auto partialX = partials[i].x;
					selection[i] = partialX >= lowerLimit && partialX <= upperLimit;
				}
			}
		};

		enum
		{
			kMaterialUpdatedCB = 0,
			kStrumCB = 1,
			kNumStrumsCB = kStrumCB + NumFilters,
			kNumCallbacks
		};

		ModalMaterialView(Utils& u, Material& _material) :
			Comp(u),
			FileDragAndDropTarget(),
			infoLabel(u, true),
			material(_material),
			partials(),
			dragAniComp(u),
			draggerfall(),
			dragXY(),
			freqRatioRange(1.f)
		{
			layout.init
			(
				{ 1 },
				{ 1, 5 }
			);

			addAndMakeVisible(infoLabel);
			makeTextLabel(infoLabel, "", font::nel(), Just::topRight, CID::Hover);

			add(Callback([&]()
			{
				if (material.status.load() == Status::UpdatedDualMaterial)
				{
					updatePartials();
					repaint();
				}
			}, kMaterialUpdatedCB, cbFPS::k30, true));

			const auto fps = cbFPS::k30;
			const auto speed = msToInc(AniLengthMs, fps);

			for(auto i = 0; i < NumFilters; ++i)
				add(Callback([&, speed, i]()
				{
					auto& phase = callbacks[kStrumCB + i].phase;
					phase -= speed;
					if (phase <= 0.f)
					{
						phase = 0.f;
						callbacks[kStrumCB + i].active = false;
					}
					repaint();
				}, kStrumCB + i, fps, false));

			addChildComponent(dragAniComp);
		}

		void paint(Graphics& g) override
		{
			auto const h = static_cast<float>(getHeight());

			if(isMouseOver() && !isMouseButtonDownAnywhere())
				draggerfall.paint(g);

			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto strumPhase = callbacks[kStrumCB + i].phase;
				const bool selected = draggerfall.isSelected(i);
				const auto knotW = utils.thicc
					* (selected ? 2.f : 1.f)
					+ utils.thicc * strumPhase * 2.f;
				const auto knotWHalf = knotW * .5f;

				const auto x = partials[i].x;
				const auto y = partials[i].y;

				if(selected)
					g.setColour(Colours::c(CID::Interact));
				else
					g.setColour(Colours::c(CID::Txt));
				g.fillRect(x - knotWHalf, y, knotW, h);

				g.setColour(Colours::c(CID::Interact));
				g.fillRect(x - knotWHalf, y - knotWHalf, knotW, knotW);
			}
		}

		void resized() override
		{
			draggerfall.resized
			(
				partials,
				static_cast<float>(getWidth()),
				static_cast<float>(getHeight())
			);

			layout.resized(getLocalBounds().toFloat());
			layout.place(infoLabel, 0, 0, 1, 1);
			dragAniComp.setBounds(getLocalBounds());
			updatePartials();
		}

		Label infoLabel;
		Material& material;
		Partials partials;
		DragAnimationComp dragAniComp;
		Draggerfall draggerfall;
		PointF dragXY;
		float freqRatioRange;
	private:

		void updatePartials()
		{
			auto const w = static_cast<float>(getWidth());
			auto const h = static_cast<float>(getHeight());

			auto maxRatio = 0.f;
			for (auto& peakInfo : material.peakInfos)
				maxRatio = std::max(maxRatio, static_cast<float>(peakInfo.ratio));
			maxRatio -= 1.f;
			const auto maxRatioInv = 1.f / maxRatio;

			for (auto i = 0; i < NumFilters; ++i)
			{
				auto& peakInfo = material.peakInfos[i];

				auto const x = ((static_cast<float>(peakInfo.ratio) - 1.f)) * maxRatioInv * w;
				auto const y = h - static_cast<float>(peakInfo.mag) * h;

				partials[i].x = x;
				partials[i].y = y;
			}

			material.status.store(Status::Processing);

			auto freqRatioMin = 44100.f;
			for (auto& peakInfo : material.peakInfos)
				freqRatioMin = std::min(freqRatioMin, static_cast<float>(peakInfo.ratio));
			auto freqRatioMax = 0.f;
			for (auto& peakInfo : material.peakInfos)
				freqRatioMax = std::max(freqRatioMax, static_cast<float>(peakInfo.ratio));
			freqRatioRange = freqRatioMax - freqRatioMin;
		}

		void mouseExit(const Mouse&) override
		{
			draggerfall.clearSelection();
			infoLabel.setText("");
			repaint();
		}

		void mouseMove(const Mouse& mouse) override
		{
			draggerfall.updateX(partials, mouse.position.x, true);
			for (auto i = 0; i < NumFilters; ++i)
			{
				const bool selected = draggerfall.isSelected(i);
				if (selected)
					callbacks[kStrumCB + i].start(1.f);
			}
			updateInfoLabel();
			repaint();
		}

		void mouseDown(const Mouse& mouse) override
		{
			if (draggerfall.selectionEmpty())
				return;

			dragXY = { static_cast<float>(mouse.x), static_cast<float>(mouse.y) };
		}

		void mouseDrag(const Mouse& mouse) override
		{
			draggerfall.updateX(partials, mouse.position.x, false);

			if (draggerfall.selectionEmpty())
				return;
			
			hideCursor();

			const auto minDimen = static_cast<float>(std::min(getWidth(), getHeight()));
			const auto speed = 3.f * utils.thicc / minDimen;
			const auto dragDist = ((mouse.position - dragXY) * speed).toDouble();

			const auto status = material.status.load();
			const auto sensitive = mouse.mods.isShiftDown();
			const auto yDepth = .4f * (sensitive ? Sensitive : 1.f);
			const auto xDepth = yDepth * freqRatioRange;

			if(status == Status::Processing)
			{
				for (auto i = 0; i < NumFilters; ++i)
				{
					const bool selected = draggerfall.isSelected(i);
					if (selected)
					{
						auto& peakInfo = material.peakInfos[i];
						peakInfo.mag = juce::jlimit(0., 100., peakInfo.mag - dragDist.y * yDepth);
						peakInfo.ratio = juce::jlimit(1., 100., peakInfo.ratio + dragDist.x * xDepth);
					}
				}
				material.updatePeakInfosFromGUI();
				updateInfoLabel();
			}
			dragXY = mouse.position;
		}

		void mouseUp(const Mouse& mouse) override
		{
			if (draggerfall.selectionEmpty())
				return;

			mouseDrag(mouse);
			material.reportEndGesture();

			for (auto i = 0; i < NumFilters; ++i)
			{
				const bool selected = draggerfall.isSelected(i);
				if (selected)
				{
					PointF pos
					(
						static_cast<float>(partials[i].x),
						static_cast<float>(partials[i].y)
					);

					PointF screenPos = localPointToGlobal(pos);

					showCursor(screenPos);
					return;
				}
			}
		}

		void mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel) override
		{
			const auto sensitive = mouse.mods.isShiftDown();
			const auto y = wheel.deltaY * (wheel.isReversed ? -1.f : 1.f);
			const auto depth = .2f * (sensitive ? Sensitive : 1.f);
			draggerfall.addLength(partials, y * depth);
			repaint();
		}

		void loadAudioFile(const File& file)
		{
			auto formatManager = std::make_unique<AudioFormatManager>();
			formatManager->registerBasicFormats();
			auto reader = formatManager->createReaderFor(file);
			if (reader != nullptr)
			{
				const auto numChannels = static_cast<int>(reader->numChannels);
				const auto numSamples = static_cast<int>(reader->lengthInSamples);
				AudioBufferF audioBuffer(numChannels, numSamples);
				reader->read(&audioBuffer, 0, numSamples, 0, true, true);
				const auto samples = audioBuffer.getArrayOfReadPointers();
				const auto sampleRate = static_cast<float>(reader->sampleRate);
				material.fillBuffer(sampleRate, samples, numChannels, numSamples);
			}
			delete reader;
		}

		void filesDropped(const StringArray& files, int, int) override
		{
			dragAniComp.stop();
			const auto status = material.status.load();
			if (status != Status::Processing)
				return;
			const auto file = files[0];
			loadAudioFile(file);
			material.load();
		}

		bool isAudioFile(const String& fileName) const
		{
			const auto ext = fileName.fromLastOccurrenceOf(".", false, false);
			return ext == "flac" || ext == "wav" || ext == "mp3" || ext == "aiff";
		}

		void fileDragEnter(const StringArray&, int, int) override
		{
			dragAniComp.start();
		}

		void fileDragExit(const StringArray&) override
		{
			dragAniComp.stop();
		}

		bool isInterestedInFileDrag(const StringArray& files) override
		{
			bool isInterested = false;

			const auto numFiles = files.size();
			if (numFiles == 1)
				if(isAudioFile(files[0]))
					isInterested = true;
				else
					dragAniComp.error = "Accepted formats: wav, flac, mp3, aiff";
			else
				dragAniComp.error = "Only one file at a time, pls.";
			dragAniComp.isInterested = isInterested;

			return isInterested;
		}

		void updateInfoLabel()
		{
			if (draggerfall.selectionEmpty())
			{
				infoLabel.setText("");
				return;
			}

			String txt("");
			auto count = 0;
			for (auto i = 0; i < NumFilters; ++i)
			{
				if (draggerfall.isSelected(i))
				{
					const auto mag = material.peakInfos[i].mag;
					const auto rat = material.peakInfos[i].ratio;
					txt += "mag: " + String(mag, 2) + ", rat : " + String(rat, 2) + "\n";
					++count;
				}
				if (count >= 3)
				{
					txt += "...";
					break;
				}
			}

			infoLabel.setText(txt);
		}
	};
}