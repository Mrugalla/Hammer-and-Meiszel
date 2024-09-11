#include "ModalFilterMaterialView.h"

namespace gui
{
	ModalMaterialView::Partial::Partial() :
		x(0.f), y(0.f)
	{
	}

	ModalMaterialView::DragAnimationComp::DragAnimationComp(Utils& u) :
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

	void ModalMaterialView::DragAnimationComp::start()
	{
		setVisible(true);
		callbacks[0].start(0.f);
	}

	void ModalMaterialView::DragAnimationComp::stop()
	{
		setVisible(false);
		callbacks[0].stop(0.f);
	}

	void ModalMaterialView::DragAnimationComp::paint(Graphics& g)
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

	ModalMaterialView::Draggerfall::Draggerfall() :
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

	void ModalMaterialView::Draggerfall::resized(Partials& partials, float w, float h) noexcept
	{
		width = w;
		height = h;
		updateX(partials, xAbs, false);
		lenAbs = juce::jlimit(1.f, width, (lenRel + 0.f) * width);
		lenRel = lenAbs / width;
		lenAbsHalf = lenAbs * .5f;
	}

	void ModalMaterialView::Draggerfall::updateX(Partials& partials, float x, bool doUpdateSelection) noexcept
	{
		xAbs = x;
		xRel = xAbs / width;
		if (doUpdateSelection)
			updateSelection(partials);
	}

	void ModalMaterialView::Draggerfall::addLength(Partials& partials, float valRel) noexcept
	{
		lenAbs = juce::jlimit(1.f, width, (lenRel + valRel) * width);
		lenRel = lenAbs / width;
		lenAbsHalf = lenAbs * .5f;
		updateSelection(partials);
	}

	void ModalMaterialView::Draggerfall::paint(Graphics& g)
	{
		const auto x = xAbs - lenAbsHalf;
		const auto w = lenAbs;
		setCol(g, CID::Hover);
		g.fillRect(x, 0.f, w, height);
	}

	bool ModalMaterialView::Draggerfall::isSelected(int i) const noexcept
	{
		return selection[i];
	}

	void ModalMaterialView::Draggerfall::clearSelection() noexcept
	{
		for (auto& s : selection)
			s = false;
	}

	bool ModalMaterialView::Draggerfall::selectionEmpty() const noexcept
	{
		for (auto s : selection)
			if (s)
				return false;
		return true;
	}

	void ModalMaterialView::Draggerfall::updateSelection(Partials& partials) noexcept
	{
		const auto lowerLimit = xAbs - lenAbsHalf;
		const auto upperLimit = lowerLimit + lenAbs;
		for (auto i = 0; i < NumFilters; ++i)
		{
			const auto partialX = partials[i].x;
			selection[i] = partialX >= lowerLimit && partialX <= upperLimit;
		}
	}

	PointF ModalMaterialView::Draggerfall::getCoords() const noexcept
	{
		return { xAbs - lenAbsHalf, lenAbs };
	}

	// ModalMaterialView

	ModalMaterialView::ModalMaterialView(Utils& u, Material& _material, Actives& _actives) :
		Comp(u),
		FileDragAndDropTarget(),
		material(_material),
		actives(_actives),
		ruler(u),
		partials(),
		dragAniComp(u),
		draggerfall(),
		dragXY(),
		freqRatioRange(1.f)
	{
		layout.init
		(
			{ 1 },
			{ 2, 13 }
		);

		initRuler();

		add(Callback([&]()
		{
			if (material.status.load() == Status::UpdatedProcessor)
			{
				updatePartials();
				repaint();
			}
		}, kMaterialUpdatedCB, cbFPS::k30, true));

		const auto fps = cbFPS::k30;
		const auto speed = msToInc(AniLengthMs, fps);

		for (auto i = 0; i < NumFilters; ++i)
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

	void ModalMaterialView::initRuler()
	{
		addAndMakeVisible(ruler);
		ruler.setGetIncFunc([](float len)
		{
			return len < 2.f ? .1f :
				len < 5.f ? .2f :
				len < 10.f ? .5f :
				len < 15.f ? 1.f :
				len < 30.f ? 2.f :
				len < 45.f ? 4.f :
				8.f;
		});
		ruler.setValToStrFunc([](float v)
		{
			return String(v + 1.f);
		});
		ruler.setCID(CID::Hover);
		ruler.setDrawFirstVal(true);
	}

	void ModalMaterialView::paint(Graphics& g)
	{
		const auto hInt = getHeight();

		const auto w = static_cast<float>(getWidth());
		auto const h = static_cast<float>(hInt);

		Gradient bgGradient
		(
			Colours::c(CID::Bg), 0.f, 0.f,
			Colours::c(CID::Darken), 0.f, h,
			false
		);
		g.setGradientFill(bgGradient);
		g.fillAll();
		const auto rulerTop = layout.getY(1);
		g.setColour(Colours::c(CID::Hover).withMultipliedAlpha(.25f));
		g.fillRect(0.f, 0.f, w, rulerTop);
		const auto rulerBtm = h;
		ruler.paintStripes(g, rulerTop, rulerBtm, 33);

		if (isMouseOver() && !isMouseButtonDownAnywhere())
			draggerfall.paint(g);

		auto unselectedPartialsCol = Colours::c(CID::Txt);
		paintPartial(g, h, unselectedPartialsCol, 0);
		if (material.soloing.load())
			unselectedPartialsCol = unselectedPartialsCol.darker(.7f);

		for (auto i = 1; i < NumFilters; ++i)
			paintPartial(g, h, unselectedPartialsCol, i);
	}

	void ModalMaterialView::paintPartial(Graphics& g, float h, Colour unselectedPartialsCol, int i)
	{
		const auto strumPhase = callbacks[kStrumCB + i].phase;
		const bool selected = draggerfall.isSelected(i);
		const auto knotW = utils.thicc
			* (selected ? 2.f : 1.f)
			+ utils.thicc * strumPhase * 2.f;
		const auto knotWHalf = knotW * .5f;

		const auto x = partials[i].x;
		const auto y = partials[i].y;

		if (selected)
			g.setColour(Colours::c(CID::Interact));
		else
			g.setColour(unselectedPartialsCol);
		g.fillRect(x - knotWHalf, y, knotW, h);

		g.setColour(Colours::c(CID::Interact));
		g.fillRect(x - knotWHalf, y - knotWHalf, knotW, knotW);
	}

	void ModalMaterialView::resized()
	{
		draggerfall.resized
		(
			partials,
			static_cast<float>(getWidth()),
			static_cast<float>(getHeight())
		);

		layout.resized(getLocalBounds().toFloat());
		layout.place(ruler, 0, 0, 1, 1);
		dragAniComp.setBounds(getLocalBounds());
		updatePartials();
	}

	void ModalMaterialView::updatePartials()
	{
		auto const w = static_cast<float>(getWidth());
		auto const h = static_cast<float>(getHeight());

		auto maxRatio = 0.f;
		for (auto p = 0; p < NumFilters; ++p)
		{
			const auto& peakInfo = material.peakInfos[p];
			const auto ratio = static_cast<float>(peakInfo.ratio);
			maxRatio = std::max(maxRatio, ratio);
		}
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
		auto freqRatioMax = 0.f;
		for (auto p = 0; p < NumFilters; ++p)
		{
			const auto& peakInfo = material.peakInfos[p];
			const auto ratio = static_cast<float>(peakInfo.ratio);
			freqRatioMin = std::min(freqRatioMin, ratio);
			freqRatioMax = std::max(freqRatioMax, ratio);
		}
			
		freqRatioRange = freqRatioMax - freqRatioMin;
		updateRuler();
	}

	void ModalMaterialView::updateRuler()
	{
		const auto& peakInfos = material.peakInfos;
		const auto minFc = 1.f;

		auto maxFc = minFc;
		for (auto i = 1; i < NumFilters; ++i)
			maxFc = std::max(maxFc, static_cast<float>(peakInfos[i].ratio));

		const auto fcRange = maxFc - minFc;
		ruler.setLength(fcRange);
		ruler.repaint();
	}

	// mouse event handling

	void ModalMaterialView::mouseEnter(const Mouse&)
	{
		const auto cID = CID::Interact;
		notify(evt::Type::ToastColour, &cID);
		const auto bounds = getBounds();
		notify(evt::Type::ToastShowUp, &bounds);
		updateInfoLabel();
	}

	void ModalMaterialView::mouseExit(const Mouse&)
	{
		draggerfall.clearSelection();
		updateInfoLabel("");
		repaint();
		notify(evt::Type::ToastVanish);
	}

	void ModalMaterialView::mouseMove(const Mouse& mouse)
	{
		draggerfall.updateX(partials, mouse.position.x, true);
		
		for (auto i = 0; i < NumFilters; ++i)
		{
			const bool selected = draggerfall.isSelected(i);
			if (selected)
				callbacks[kStrumCB + i].start(1.f);
		}
		updateActives(material.soloing.load());

		updateInfoLabel();
		repaint();
	}

	void ModalMaterialView::mouseDown(const Mouse& mouse)
	{
		if (draggerfall.selectionEmpty())
			return;

		dragXY = { static_cast<float>(mouse.x), static_cast<float>(mouse.y) };
	}

	void ModalMaterialView::mouseDrag(const Mouse& mouse)
	{
		draggerfall.updateX(partials, mouse.position.x, false);

		if (draggerfall.selectionEmpty())
			return;

		hideCursor();

		const auto minDimen = static_cast<float>(std::min(getWidth(), getHeight()));
		const auto speed = 3.f * utils.thicc / minDimen;
		const auto dragDist = ((mouse.position - dragXY) * speed).toDouble();

		const auto status = material.status.load();
		const auto sensitive = juce::ComponentPeer::getCurrentModifiersRealtime().isShiftDown();
		const auto yDepth = .4f * (sensitive ? Sensitive : 1.f);
		const auto xDepth = yDepth * freqRatioRange * .5f;

		if (status == Status::Processing)
		{
			if (draggerfall.isSelected(0))
			{
				auto& peakInfo = material.peakInfos[0];
				peakInfo.mag = juce::jlimit(0., 100., peakInfo.mag - dragDist.y * yDepth);
			}
			for (auto i = 1; i < NumFilters; ++i)
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

	void ModalMaterialView::mouseUp(const Mouse& mouse)
	{
		if (!draggerfall.selectionEmpty())
		{
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
					break;
				}
			}
		}
	}

	void ModalMaterialView::mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel)
	{
		const auto sensitive = mouse.mods.isShiftDown();
		const auto y = wheel.deltaY * (wheel.isReversed ? -1.f : 1.f);
		const auto depth = .2f * (sensitive ? Sensitive : 1.f);
		draggerfall.addLength(partials, y * depth);
		repaint();
	}

	// related to drag n drop and audio file handling

	bool ModalMaterialView::isAudioFile(const String& fileName) const
	{
		const auto ext = fileName.fromLastOccurrenceOf(".", false, false).toLowerCase();
		return ext == "flac" || ext == "wav" || ext == "mp3" || ext == "aiff";
	}

	void ModalMaterialView::loadAudioFile(const File& file)
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

	void ModalMaterialView::filesDropped(const StringArray& files, int, int)
	{
		dragAniComp.stop();
		const auto status = material.status.load();
		if (status != Status::Processing)
			return;
		const auto file = files[0];
		loadAudioFile(file);
		material.load();
	}

	void ModalMaterialView::fileDragEnter(const StringArray&, int, int)
	{
		dragAniComp.start();
	}

	void ModalMaterialView::fileDragExit(const StringArray&)
	{
		dragAniComp.stop();
	}

	bool ModalMaterialView::isInterestedInFileDrag(const StringArray& files)
	{
		bool isInterested = false;

		const auto numFiles = files.size();
		if (numFiles == 1)
			if (isAudioFile(files[0]))
				isInterested = true;
			else
				dragAniComp.error = "Accepted formats: wav, flac, mp3, aiff";
		else
			dragAniComp.error = "Only one file at a time, pls.";
		dragAniComp.isInterested = isInterested;

		return isInterested;
	}

	void ModalMaterialView::updateInfoLabel(const String& nMessage)
	{
		if (nMessage != "abcabcabc")
		{
			notify(evt::Type::ToastUpdateMessage, &nMessage);
			return;
		}

		if (draggerfall.selectionEmpty())
		{
			String empty("");
			notify(evt::Type::ToastUpdateMessage, &empty);
			return;
		}

		String txt("");
		for(auto i = 0; i < NumFilters; ++i)
			if (draggerfall.isSelected(i))
			{
				const auto mag = material.peakInfos[i].mag;
				const auto rat = material.peakInfos[i].ratio;
				txt += "MG: " + String(mag, 1) + "\nFC : " + String(rat, 1) + "\n";
				break;
			}

		notify(evt::Type::ToastUpdateMessage, &txt);
	}

	void ModalMaterialView::updateActives(bool soloActive)
	{
		bool wantMaterialUpdate = false;
		if (soloActive)
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				const bool selected = draggerfall.isSelected(i);
				if (actives[i] != selected)
				{
					wantMaterialUpdate = true;
					actives[i] = selected;
				}
			}
		}
		else
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				if (!actives[i])
				{
					wantMaterialUpdate = true;
					actives[i] = true;
				}
			}
		}
		if (wantMaterialUpdate)
			material.reportUpdate();
	}
}