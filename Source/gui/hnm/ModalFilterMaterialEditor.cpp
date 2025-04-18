#include "ModalFilterMaterialEditor.h"

namespace gui
{
	ModalMaterialEditor::Partial::Partial() :
		x(0.f), y(0.f)
	{
	}

	ModalMaterialEditor::DragAnimationComp::DragAnimationComp(Utils& u) :
		Comp(u),
		isInterested(false),
		error("")
	{
		setInterceptsMouseClicks(false, false);

		const auto fps = cbFPS::k15;
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

	void ModalMaterialEditor::DragAnimationComp::start()
	{
		setVisible(true);
		callbacks[0].start(0.f);
	}

	void ModalMaterialEditor::DragAnimationComp::stop()
	{
		setVisible(false);
		callbacks[0].stop(0.f);
	}

	void ModalMaterialEditor::DragAnimationComp::paint(Graphics& g)
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

	ModalMaterialEditor::Draggerfall::Draggerfall() :
		coordsAbs(-1.f, -1.f),
		coordsRel(0.f, 0.f),
		width(1.f),
		height(1.f),
		radAbs(.1f),
		radRel(1.f / Pi),
		selection()
	{
		for (auto& s : selection)
			s = false;
	}

	void ModalMaterialEditor::Draggerfall::resized(Partials& partials, float w, float h) noexcept
	{
		width = w;
		height = h;
		updateCoords(coordsAbs);
		updateRadius(partials, 0.f);
	}

	void ModalMaterialEditor::Draggerfall::updateCoords(PointF xy) noexcept
	{
		coordsAbs = xy;
		coordsRel = { coordsAbs.x / width, coordsAbs.y / height };
	}

	void ModalMaterialEditor::Draggerfall::updateRadius(Partials& partials, float addToRadiusRel) noexcept
	{
		const auto minDimen = std::min(width, height);
		radRel = juce::jlimit(.1f, 1.5f, radRel + addToRadiusRel);
		radAbs = radRel * minDimen;
		if (coordsAbs.getX() < 0.f)
			return;
		updateSelection(partials);
	}

	void ModalMaterialEditor::Draggerfall::paint(Graphics& g, float margin)
	{
		const auto radAbs2 = radAbs * 2.f;
		const BoundsF bounds
		(
			coordsAbs.x - radAbs,
			coordsAbs.y - radAbs,
			radAbs2,
			radAbs2
		);
		setCol(g, CID::Hover, .5f);
		g.fillEllipse(bounds);
		g.fillEllipse(bounds.reduced(margin));
	}

	bool ModalMaterialEditor::Draggerfall::isSelected(int i) const noexcept
	{
		return selection[i];
	}

	void ModalMaterialEditor::Draggerfall::clearSelection() noexcept
	{
		for (auto& s : selection)
			s = false;
	}

	bool ModalMaterialEditor::Draggerfall::selectionEmpty() const noexcept
	{
		for (auto s : selection)
			if (s)
				return false;
		return true;
	}

	void ModalMaterialEditor::Draggerfall::updateSelection(Partials& partials) noexcept
	{
		for (auto i = 0; i < NumPartials; ++i)
		{
			const auto& partial = partials[i];
			const PointF posAbs(partial.x, partial.y);
			const LineF line(coordsAbs, posAbs);
			selection[i] = line.getLength() < radAbs;
		}
	}

	PointF ModalMaterialEditor::Draggerfall::getCoords() const noexcept
	{
		return coordsAbs;
	}

	// ModalMaterialEditor

	ModalMaterialEditor::ModalMaterialEditor(Utils& u, Material& _material, Actives& _actives) :
		Comp(u),
		FileDragAndDropTarget(),
		material(_material),
		actives(_actives),
		ruler(u),
		partials(),
		dragAniComp(u),
		draggerfall(),
		dragXY(),
		freqRatioRange(1.f),
		xenInfo(u.audioProcessor.xenManager.getInfo())
	{
		layout.init
		(
			{ 1 },
			{ 1, 13 }
		);

		initRuler();

		add(Callback([&]()
		{
			if (material.status.load() != Status::UpdatedProcessor)
				return;
			updatePartials();
			repaint();
		}, kMaterialUpdatedCB, cbFPS::k60, true));

		const auto fps = cbFPS::k30;
		const auto speed = msToInc(AniLengthMs, fps);

		for (auto i = 0; i < NumPartials; ++i)
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

		add(Callback([&]()
		{
			const auto& nInfo = utils.audioProcessor.xenManager.getInfo();
			if (nInfo == xenInfo)
				return;
			xenInfo = nInfo;
			updateRuler();
			repaint();
		}, kXenUpdatedCB, cbFPS::k7_5, true));

		addChildComponent(dragAniComp);
	}

	void ModalMaterialEditor::initRuler()
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
				len < 60.f ? 8.f :
				16.f;
		});
		ruler.setValToStrFunc([](float v)
		{
			return String(v + 1.f);
		});
		ruler.setCID(CID::Hover);
		ruler.setDrawFirstVal(true);
	}

	void ModalMaterialEditor::paint(Graphics& g)
	{
		const auto hInt = getHeight();

		const auto w = static_cast<float>(getWidth());
		auto const h = static_cast<float>(hInt);
		const auto thicc = utils.thicc;
		const auto thicc2 = thicc * 2.f;

		Gradient bgGradient
		(
			Colours::c(CID::Bg), 0.f, 0.f,
			Colours::c(CID::Darken), 0.f, h,
			false
		);
		g.setGradientFill(bgGradient);
		g.fillAll();
		const auto rulerTop = layout.getY(1);
		g.setColour(Colours::c(CID::Hover).withMultipliedAlpha(.15f));
		g.fillRect(0.f, 0.f, w, rulerTop);
		const auto rulerBtm = h;
		ruler.paintStripes(g, rulerTop, rulerBtm, 33);

		if (isMouseOver() && !isMouseButtonDownAnywhere())
			draggerfall.paint(g, thicc2);

		auto unselectedPartialsCol = Colours::c(CID::Txt);
		paintPartial(g, h, unselectedPartialsCol, 0);
		if (material.soloing.load())
			unselectedPartialsCol = unselectedPartialsCol.darker(.7f);

		for (auto i = 1; i < NumPartials; ++i)
			paintPartial(g, h, unselectedPartialsCol, i);
	}

	void ModalMaterialEditor::paintPartial(Graphics& g, float h, Colour unselectedPartialsCol, int i)
	{
		const auto strumPhase = callbacks[kStrumCB + i].phase;
		const bool selected = draggerfall.isSelected(i);
		const auto knotW = utils.thicc
			* (selected ? 1.5f : 1.f)
			+ utils.thicc * strumPhase * 2.f;
		const auto knotWHalf = knotW * .5f;
		const auto knotW2 = knotW * 2.f;

		const auto x = partials[i].x;
		const auto y = partials[i].y;

		if (selected)
			g.setColour(Colours::c(CID::Interact));
		else
			g.setColour(unselectedPartialsCol);
		g.fillRect(x - knotWHalf, y, knotW, h);

		g.setColour(Colours::c(CID::Interact));
		g.fillEllipse(x - knotW, y - knotW, knotW2, knotW2);
	}

	void ModalMaterialEditor::resized()
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

	void ModalMaterialEditor::updatePartials()
	{
		updatePartialsRatios();

		material.status.store(Status::Processing);

		auto freqRatioMin = 44100.f;
		auto freqRatioMax = 0.f;
		for (auto p = 0; p < NumPartials; ++p)
		{
			const auto& peakInfo = material.peakInfos[p];
			const auto ratio = static_cast<float>(peakInfo.fc);
			freqRatioMin = std::min(freqRatioMin, ratio);
			freqRatioMax = std::max(freqRatioMax, ratio);
		}

		freqRatioRange = freqRatioMax - freqRatioMin;
		updateRuler();
	}

	void ModalMaterialEditor::updatePartialsRatios()
	{
		auto const w = static_cast<float>(getWidth());
		auto const h = static_cast<float>(getHeight());

		auto maxRatio = 0.f;
		for (auto p = 0; p < NumPartials; ++p)
		{
			const auto& peakInfo = material.peakInfos[p];
			const auto ratio = static_cast<float>(peakInfo.fc);
			maxRatio = std::max(maxRatio, ratio);
		}
		maxRatio -= 1.f;
		const auto maxRatioInv = 1.f / maxRatio;

		for (auto i = 0; i < NumPartials; ++i)
		{
			auto& peakInfo = material.peakInfos[i];

			auto const x = ((static_cast<float>(peakInfo.fc) - 1.f)) * maxRatioInv * w;
			auto const y = h - static_cast<float>(peakInfo.mag) * h;

			partials[i].x = x;
			partials[i].y = y;
		}
	}

	void ModalMaterialEditor::updateRuler()
	{
		const auto& peakInfos = material.peakInfos;
		const auto minFc = 1.f;
		auto maxFc = minFc;
		for (auto i = 0; i < NumPartials; ++i)
			maxFc = std::max(maxFc, static_cast<float>(peakInfos[i].fc));

		const auto fcRange = maxFc - minFc;
		ruler.setLength(fcRange);
		ruler.repaint();
	}

	// mouse event handling

	void ModalMaterialEditor::mouseEnter(const Mouse&)
	{
		const auto cID = CID::Interact;
		notify(evt::Type::ToastColour, &cID);
		const auto screenBoundsPlugin = utils.pluginTop.getScreenBounds();
		const auto screenBounds = getScreenBounds();
		const auto boundsInPlugin = screenBounds - screenBoundsPlugin.getTopLeft();
		notify(evt::Type::ToastShowUp, &boundsInPlugin);
		updateInfoLabel();
	}

	void ModalMaterialEditor::mouseExit(const Mouse&)
	{
		draggerfall.clearSelection();
		updateInfoLabel("");
		repaint();
		notify(evt::Type::ToastVanish);
	}

	void ModalMaterialEditor::mouseMove(const Mouse& mouse)
	{
		draggerfall.updateCoords(mouse.position);
		draggerfall.updateSelection(partials);
		
		for (auto i = 0; i < NumPartials; ++i)
		{
			const bool selected = draggerfall.isSelected(i);
			if (selected)
				callbacks[kStrumCB + i].start(1.f);
		}
		updateActives(material.soloing.load());

		updateInfoLabel();
		repaint();
	}

	void ModalMaterialEditor::mouseDown(const Mouse& mouse)
	{
		notify(evt::Type::ClickedEmpty);
		if (draggerfall.selectionEmpty())
			return;

		dragXY = { static_cast<float>(mouse.x), static_cast<float>(mouse.y) };
	}

	void ModalMaterialEditor::mouseDrag(const Mouse& mouse)
	{
		draggerfall.updateCoords(mouse.position);
		if (draggerfall.selectionEmpty())
			return;

		hideCursor();

		const auto minDimen = static_cast<float>(std::min(getWidth(), getHeight()));
		const auto speed = 4.f * utils.thicc / minDimen;
		const auto dragDist = ((mouse.position - dragXY) * speed).toDouble();

		const auto status = material.status.load();
		const auto sensitive = juce::ComponentPeer::getCurrentModifiersRealtime().isShiftDown();
		const auto yDepth = .4 * (sensitive ? Sensitive : 1.);
		const auto xDepth = yDepth * freqRatioRange * .5f;

		if (status == Status::Processing)
		{
			if (draggerfall.isSelected(0))
			{
				auto& peakInfo = material.peakInfos[0];
				peakInfo.mag = juce::jlimit(0., 2., peakInfo.mag - dragDist.y * yDepth);
			}
			for (auto i = 1; i < NumPartials; ++i)
			{
				const bool selected = draggerfall.isSelected(i);
				if (selected)
				{
					auto& peakInfo = material.peakInfos[i];
					peakInfo.mag = juce::jlimit(0., 100., peakInfo.mag - dragDist.y * yDepth);
					auto ratio = peakInfo.fc;
					ratio += dragDist.x * xDepth;
					peakInfo.fc = juce::jlimit(1., 420., ratio);
				}
			}
			material.updatePeakInfosFromGUI();
			updateInfoLabel();
		}

		dragXY = mouse.position;
	}

	void ModalMaterialEditor::mouseUp(const Mouse& mouse)
	{
		if (!draggerfall.selectionEmpty())
		{
			mouseDrag(mouse);
			material.reportEndGesture();

			for (auto i = 0; i < NumPartials; ++i)
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

	void ModalMaterialEditor::mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel)
	{
		const auto sensitive = mouse.mods.isShiftDown();
		const auto y = wheel.deltaY * (wheel.isReversed ? -1.f : 1.f);
		const bool snap = mouse.mods.isAltDown();
		if (snap)
			return mouseWheelSnap(y > 0.);
		const auto depth = .15 * (sensitive ? Sensitive : 1.);
		draggerfall.updateRadius(partials, y * static_cast<float>(depth));
		repaint();
	}

	void ModalMaterialEditor::mouseWheelSnap(bool goingUp)
	{
		for(auto i = 1; i < NumPartials; ++i)
		{
			const bool selected = draggerfall.isSelected(i);
			if (selected)
			{
				auto& peakInfo = material.peakInfos[i];
				auto fc = peakInfo.fc;
				if (goingUp)
				{
					fc = ruler.getNextHigherSnapped(fc);
					if (fc >= 420.)
						return;
				}
				else
				{
					fc = ruler.getNextLowerSnapped(fc);
					if (fc <= 1.)
						return;
				}
				peakInfo.fc = fc;
			}
		}

		material.reportEndGesture();
	}

	void ModalMaterialEditor::updateInfoLabel(const String& nMessage)
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
		for (auto i = 0; i < NumPartials; ++i)
			if (draggerfall.isSelected(i))
			{
				const auto mag = material.peakInfos[i].mag;
				const auto rat = material.peakInfos[i].fc;
				txt += "MG: " + String(std::round(mag * 100.f)) + "%\nFC : " + String(rat, 1) + "\n";
				i = NumPartials;
			}
		
		notify(evt::Type::ToastUpdateMessage, &txt);
	}

	void ModalMaterialEditor::updateActives(bool soloActive)
	{
		bool wantMaterialUpdate = false;
		if (soloActive)
		{
			for (auto i = 0; i < NumPartials; ++i)
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
			for (auto i = 0; i < NumPartials; ++i)
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

	// FileDragAndDropTarget

	bool ModalMaterialEditor::isAudioFile(const String& fileName) const
	{
		const auto ext = fileName.fromLastOccurrenceOf(".", false, false).toLowerCase();
		return ext == "flac" || ext == "wav" || ext == "mp3" || ext == "aiff";
	}

	void ModalMaterialEditor::loadAudioFile(const File& file)
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

	void ModalMaterialEditor::filesDropped(const StringArray& files, int, int)
	{
		dragAniComp.stop();
		const auto status = material.status.load();
		if (status != Status::Processing)
			return;
		const auto file = files[0];
		loadAudioFile(file);
		material.load();
	}

	void ModalMaterialEditor::fileDragEnter(const StringArray&, int, int)
	{
		dragAniComp.start();
	}

	void ModalMaterialEditor::fileDragExit(const StringArray&)
	{
		dragAniComp.stop();
	}

	bool ModalMaterialEditor::isInterestedInFileDrag(const StringArray& files)
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

	// DragAndDropTarget

	bool ModalMaterialEditor::isInterestedInDragSource(const SourceDetails& src)
	{
		const auto desc = src.description.toString();
		if (desc == "pluginrecorder")
		{
			const auto file = getTheDnDFile();
			if (file.existsAsFile())
			{
				dragAniComp.isInterested = true;
				dragAniComp.start();
				return true;
			}
		}
		dragAniComp.isInterested = false;
		return false;
	}

	void ModalMaterialEditor::itemDropped(const SourceDetails& src)
	{
		dragAniComp.stop();
		if (!isInterestedInDragSource(src))
			return;
		const auto file = getTheDnDFile();
		if (!file.existsAsFile())
			return;
		loadAudioFile(file);
		material.load();
		dragAniComp.stop();
	}

	void ModalMaterialEditor::itemDragExit(const SourceDetails&)
	{
		dragAniComp.stop();
	}

	const File ModalMaterialEditor::getTheDnDFile() const
	{
		const auto& user = *utils.audioProcessor.state.props.getUserSettings();
		const auto settingsFile = user.getFile();
		const auto userDirectory = settingsFile.getParentDirectory();
		return userDirectory.getChildFile("HnM.wav");
	}
}