#include "Knob.h"

namespace gui
{
    // Knob

    Knob::Knob(Utils& u) :
        Comp(u),
        values(),
        onEnter([]() {}), onExit([]() {}), onDown([]() {}), onDoubleClick([]() {}),
        onDrag([](const PointF&, const Mouse&) {}),
        onUp([](const Mouse&) {}), onWheel([](const Mouse&) {}),
        onPaint([](Graphics& g, Knob&) { g.fillAll(juce::Colours::red); }),
        dragXY(), lastPos(),
        hidesCursor(true)
    {
        const auto fps = cbFPS::k60;
        const auto speed = msToInc(AniLengthMs, fps);

        add(Callback([&, speed]()
            {
                auto& phase = callbacks[kEnterExitCB].phase;
                const auto pol = isMouseOverOrDragging() ? 1.f : -1.f;
                phase += speed * pol;
                if (phase >= 1.f)
                    phase = 1.f;
                else if (phase < 0.f)
                    callbacks[kEnterExitCB].stop(0.f);
                repaint();
            }, kEnterExitCB, fps, false));

        add(Callback([&, speed]()
            {
                auto& phase = callbacks[kDownUpCB].phase;
                const auto pol = isMouseButtonDown() ? 1.f : -1.f;
                phase += speed * pol;
                if (phase >= 1.f)
                    phase = 1.f;
                else if (phase < 0.f)
                    callbacks[kDownUpCB].stop(0.f);
                repaint();
            }, kDownUpCB, fps, false));
    }

    void Knob::mouseEnter(const Mouse& mouse)
    {
        Comp::mouseEnter(mouse);
        if (!callbacks[kEnterExitCB].active)
            callbacks[kEnterExitCB].start(0.f);
        onEnter();
    }

    void Knob::mouseExit(const Mouse& mouse)
    {
        Comp::mouseExit(mouse);
        onExit();
    }

    void Knob::mouseDown(const Mouse& mouse)
    {
        utils.giveDAWKeyboardFocus();
        callbacks[kDownUpCB].start(0.f);

        dragXY.setXY
        (
            mouse.position.x,
            mouse.position.y
        );
        lastPos = getPosition().toFloat();

        onDown();
    }

    void Knob::mouseDrag(const Mouse& mouse)
    {
        if (hidesCursor)
            hideCursor();

        const auto nPosition = getPosition().toFloat();
        const auto posShift = nPosition - lastPos;
        lastPos = nPosition;

        dragXY -= posShift;
        const auto shiftDown = juce::ComponentPeer::getCurrentModifiersRealtime().isShiftDown();
        const auto dragSpeed = shiftDown ? SensitiveDrag : 1.f;
        const auto dragOffset = (mouse.position - dragXY) * dragSpeed;
        onDrag(dragOffset, mouse);
        dragXY = mouse.position;
    }

    void Knob::mouseUp(const Mouse& mouse)
    {
        onUp(mouse);

        if (hidesCursor && mouse.mouseWasDraggedSinceMouseDown())
            showCursor(*this);
    }

    void Knob::mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel)
    {
        if (mouse.mods.isAnyMouseButtonDown())
            return;

        const auto deltaY = wheel.deltaY;
        const auto deltaYSqr = deltaY * deltaY;
        const bool isTrackPad = deltaYSqr < .0549316f;
        const auto shift = juce::ComponentPeer::getCurrentModifiersRealtime().isShiftDown();
        auto speed = isTrackPad ? -deltaY : WheelSpeed * (deltaY > 0.f ? 1.f : -1.f);
        speed *= shift ? SensitiveDrag : 1.f;
        speed *= wheel.isReversed ? -1.f : 1.f;
        dragXY.y = speed;

        onWheel(mouse);
    }

    void Knob::mouseDoubleClick(const Mouse& mouse)
    {
        Comp::mouseDoubleClick(mouse);
        onDoubleClick();
    }

    void Knob::paint(Graphics& g)
    {
        onPaint(g, *this);
    }

    // ModDial

    void updateToast(Utils& utils, Param& paramMain)
    {
        String msg;
        auto& macro = utils.getParam(PID::Macro);
        const auto macroVal = macro.getValue();
        const auto valMain = paramMain.getValue();
        const auto valMod = paramMain.calcValModOf(macroVal);
        const auto valModNorm = math::limit(0.f, 1.f, valMain + valMod);
        msg += paramMain.getText(valModNorm, 1);
        utils.eventSystem.notify(evt::Type::ToastUpdateMessage, &msg);
    }

    ModDial::ModDial(Utils& u) :
        Knob(u),
        prms(),
        path(),
        showBias(false),
        verticalDrag(true)
    {
        onEnter = [&]()
        {
            const auto cID = CID::Mod;
            notify(evt::Type::ToastColour, &cID);
            const auto windowPos = utils.pluginTop.getScreenPosition();
            auto knobBounds = getScreenBounds();
			knobBounds.setX(knobBounds.getX() - windowPos.x);
			knobBounds.setY(knobBounds.getY() - windowPos.y);
            notify(evt::Type::ToastShowUp, &knobBounds);
            updateToast(utils, *prms[0]);
        };

        onExit = [&]()
        {
            notify(evt::Type::ToastVanish);
        };

        onDrag = [&](const PointF& dragOffset, const Mouse& mouse)
        {

            auto dragXY = verticalDrag ? -dragOffset.y : dragOffset.x;
            const auto speed = 1.f / utils.getDragSpeed();
            dragXY *= speed;

            if (mouse.mods.isRightButtonDown())
            {
                for (auto prm : prms)
                {
                    const auto md = prm->getModDepth();
                    const auto pol = md > 0.f ? 1.f : -1.f;
                    const auto bias = prm->getModBias();
                    prm->setModBias(bias + dragXY * pol);
                }

                showBias = true;
                updateToast(utils, *prms[0]);
            }
            else
            {
                for (auto prm : prms)
                    prm->setModDepth(prm->getModDepth() + dragXY);
                showBias = false;
                updateToast(utils, *prms[0]);
            }
        };

        onUp = [&](const Mouse& mouse)
        {
            if (mouse.mouseWasDraggedSinceMouseDown())
                return;

            if (mouse.mods.isAltDown())
                for (auto prm : prms)
                    prm->setModulationDefault();
        };

        onDoubleClick = [&]()
        {
            for (auto prm : prms)
                prm->setModulationDefault();
        };

        onWheel = [&](const Mouse&)
        {
            for (auto prm : prms)
            {
                auto& param = *prm;
                const auto& range = param.range;
                const auto interval = range.interval;

                if (interval > 0.f)
                {
                    const auto nStep = interval / range.getRange().getLength();
                    const auto nY = dragXY.y > 0.f ? nStep : -nStep;
                    dragXY.setY(nY);
                }

				const auto modDepth = param.getModDepth();
				const auto valNorm = param.getValue();

				const auto nModDepth = modDepth + dragXY.y;
				const auto nValNorm = valNorm + nModDepth;
				const auto nValDenorm = range.convertFrom0to1(nValNorm);
				const auto valLegal = range.snapToLegalValue(nValDenorm);
				const auto valLegalNorm = range.convertTo0to1(valLegal);
				const auto modDepthLegal = valLegalNorm - valNorm;
               param.setModDepth(modDepthLegal);
            }
            updateToast(utils, *prms[0]);
        };
    }

    void ModDial::attach(PID* pIDs, int numPIDs)
    {
        prms.clear();
        prms.reserve(numPIDs);
        for (auto i = 0; i < numPIDs; ++i)
            prms.emplace_back(&utils.getParam(pIDs[i]));
    }

    void ModDial::attach(PID pID)
    {
        attach(&pID, 1);
    }

    void ModDial::paint(Graphics& g)
    {
        const auto enterExitPhase = callbacks[kEnterExitCB].phase;

        auto bounds = maxQuadIn(getLocalBounds().toFloat());

        g.setColour(getColour(CID::Mod));
        g.fillEllipse(bounds);

        const auto w = static_cast<float>(bounds.getWidth());
        const auto wOff = w * .2f;
        bounds = bounds.reduced(wOff);

        path.clear();
        const auto& mainParam = *prms[0];
        const auto bias = mainParam.getModBias();

        const auto left = bounds.getX();
        const auto btm = bounds.getBottom();
        const auto right = bounds.getRight();
        const auto top = bounds.getY();
        const auto width = bounds.getWidth();
        auto y = Param::biased(btm, top, bias, 0.f);
        path.startNewSubPath(left, btm);

        const auto widthInv = 1.f / width;
        for (auto i = 0.f; i < width; ++i)
        {
            const auto x = left + i;
            const auto ratio = i * widthInv;
            y = Param::biased(btm, top, bias, ratio);
            path.lineTo(x, y);
        }

        path.lineTo(right, top);

        Stroke stroke(wOff * (.5f + .5f * enterExitPhase), Stroke::JointStyle::curved, Stroke::EndCapStyle::butt);
        g.setColour(getColour(CID::Bg));
        g.strokePath(path, stroke);
    }

    // Parameters and Look&Feel

    void makeParameters(const std::vector<PID>& pIDs, Knob& knob, bool verticalDrag)
    {
        knob.setTooltip(toTooltip(pIDs[0]));

        std::vector<Param*> prms;
        prms.reserve(pIDs.size());
        for (auto i = 0; i < pIDs.size(); ++i)
            prms.emplace_back(&knob.utils.getParam(pIDs[i]));

        knob.onEnter = [&k = knob, prm = prms[0]]()
        {
            const auto cID = CID::Interact;
            k.notify(evt::Type::ToastColour, &cID);
            const auto windowPos = k.utils.pluginTop.getScreenPosition();
            auto knobBounds = k.getScreenBounds();
            knobBounds.setX(knobBounds.getX() - windowPos.x);
            knobBounds.setY(knobBounds.getY() - windowPos.y);
            k.notify(evt::Type::ToastShowUp, &knobBounds);
            const String message(prm->getCurrentValueAsText());
            k.notify(evt::Type::ToastUpdateMessage, &message);
        };

        knob.onExit = [&k = knob]()
        {
            k.notify(evt::Type::ToastVanish);
        };

        knob.onDown = [&k = knob, prms]()
        {
            for (auto prm : prms)
                if (!prm->isInGesture())
                    prm->beginGesture();

            const String message(prms[0]->getCurrentValueAsText());
            k.notify(evt::Type::ToastUpdateMessage, &message);
        };

        knob.onDrag = [&k = knob, prms, verticalDrag](const PointF& dragOffset, const Mouse&)
        {
            auto dragVal = verticalDrag ? -dragOffset.y : dragOffset.x;
            const auto speed = 1.f / k.utils.getDragSpeed();
            dragVal *= speed;

            for (auto prm : prms)
            {
                const auto newValue = juce::jlimit(0.f, 1.f, prm->getValue() + dragVal);
                prm->setValueFromEditor(newValue);
            }

            const String message(prms[0]->getCurrentValueAsText());
            k.notify(evt::Type::ToastUpdateMessage, &message);
        };

        knob.onUp = [&k = knob, prms](const Mouse& mouse)
        {
            if (!mouse.mouseWasDraggedSinceMouseDown())
                if (mouse.mods.isAltDown())
                    for (auto prm : prms)
                        prm->setValueFromEditor(prm->getDefaultValue());

            for (auto prm : prms)
                prm->endGesture();

            const String message(prms[0]->getCurrentValueAsText());
            k.notify(evt::Type::ToastUpdateMessage, &message);
        };

        knob.onWheel = [&k = knob, prms](const Mouse&)
        {
            for (auto prm : prms)
            {
                const auto& range = prm->range;
                const auto interval = range.interval;

                if (interval > 0.f)
                {
                    const auto nStep = interval / range.getRange().getLength();
                    k.dragXY.setY(k.dragXY.y > 0.f ? nStep : -nStep);
                    auto newValue = juce::jlimit(0.f, 1.f, prm->getValue() + k.dragXY.y);
                    newValue = range.convertTo0to1(range.snapToLegalValue(range.convertFrom0to1(newValue)));
                    prm->setValueWithGesture(newValue);
                }
                else
                {
                    const auto newValue = juce::jlimit(0.f, 1.f, prm->getValue() + k.dragXY.y);
                    prm->setValueWithGesture(newValue);
                }
            }

            const String message(prms[0]->getCurrentValueAsText());
            k.notify(evt::Type::ToastUpdateMessage, &message);
        };

        knob.onDoubleClick = [&k = knob, prms]()
        {
            for (auto prm : prms)
                if (!prm->isInGesture())
                    prm->setValueWithGesture(prm->getDefaultValue());

            const String message(prms[0]->getCurrentValueAsText());
            k.notify(evt::Type::ToastUpdateMessage, &message);
        };

        knob.values.resize(Knob::kVals::NumValTypes, 0.f);

        knob.add(Callback([&k = knob, &prm = *prms[0]]()
        {
            bool shallRepaint = false;

            k.setLocked(prm.isLocked());

            const auto vn = prm.getValue();
            const auto md = prm.getModDepth();
            const auto vm = prm.getValMod();
            const auto mb = prm.getModBias();

            auto& vals = k.values;

            if (vals[Knob::kVals::Value] != vn || vals[Knob::kVals::ModDepth] != md ||
                vals[Knob::kVals::ValMod] != vm || vals[Knob::kVals::ModBias] != mb)
            {
                vals[Knob::kVals::Value] = vn;
                vals[Knob::kVals::ModDepth] = md;
                vals[Knob::kVals::ValMod] = vm;
                vals[Knob::kVals::ModBias] = mb;
                shallRepaint = true;
            }

            if (shallRepaint)
                k.repaint();
        }, Knob::kCBs::kUpdateParameterCB, cbFPS::k60, true));

        // still to do:
        // label behaviour
    }

    void makeParameter(PID pID, Knob& knob, bool verticalDrag)
    {
        makeParameters({ pID }, knob, verticalDrag);
    }

    void makeKnob(Knob& knob, bool showModulation)
    {
        knob.onPaint = [showModulation](Graphics& g, Knob& k)
            {
                static constexpr float AngleWidth = PiQuart * 3.f;
                static constexpr float AngleRange = AngleWidth * 2.f;

                const auto& vals = k.values;

                const auto enterExitPhase = k.callbacks[Knob::kEnterExitCB].phase;
                const auto downUpPhase = k.callbacks[Knob::kDownUpCB].phase;

                const auto thicc = k.utils.thicc;
                const auto thicc2 = thicc * 2.f;
                const auto thicc3 = thicc * 3.f;
                const auto thicc4 = thicc * 4.f;
                const auto thicc5 = thicc * 5.f;
                const auto knobBounds = maxQuadIn(k.getLocalBounds().toFloat()).reduced(thicc3);

                const auto radius = knobBounds.getWidth() * .5f;
                const auto radiusInner = radius * (.8f - enterExitPhase * .1f);
                const auto radDif = (radius - radiusInner) * (.7f + downUpPhase * .3f);
                Stroke strokeType(radDif * .5f, Stroke::JointStyle::curved, Stroke::EndCapStyle::butt);

                PointF centre
                (
                    radius + knobBounds.getX(),
                    radius + knobBounds.getY()
                );

                auto col = getColour(CID::Txt);

                { // paint lines

                    Path arcOutline;
                    arcOutline.addCentredArc
                    (
                        centre.x, centre.y,
                        radius, radius,
                        0.f,
                        -AngleWidth, AngleWidth,
                        true
                    );
                    g.setColour(col);
                    g.strokePath(arcOutline, strokeType);

                    Path arcInline;
                    arcInline.addCentredArc
                    (
                        centre.x, centre.y,
                        radiusInner, radiusInner,
                        0.f,
                        -AngleWidth, AngleWidth,
                        true
                    );
                    Stroke stroke2 = strokeType;
                    stroke2.setStrokeThickness(radDif);
                    g.strokePath(arcInline, stroke2);
                };

                const auto valNormAngle = vals[Knob::Value] * AngleRange;
                const auto valAngle = -AngleWidth + valNormAngle;
                const auto radiusExt = radius + thicc;

                // paint modulation
                if (showModulation)
                {
                    const auto valModAngle = vals[Knob::ValMod] * AngleRange;
                    const auto modAngle = -AngleWidth + valModAngle;
                    const auto modTick = LineF::fromStartAndAngle(centre, radiusExt, modAngle);
                    const auto shortenedModTick = modTick.withShortenedStart(radiusInner - thicc);

                    g.setColour(Colours::c(CID::Bg));
                    g.drawLine(shortenedModTick, thicc4);

                    const auto maxModDepthAngle = juce::jlimit(-AngleWidth, AngleWidth, valNormAngle + vals[Knob::ModDepth] * AngleRange - AngleWidth);

                    g.setColour(Colours::c(CID::Mod));
                    g.drawLine(modTick.withShortenedStart(radiusInner), thicc2);
                    {
                        Path modPath;
                        modPath.addCentredArc
                        (
                            centre.x, centre.y,
                            radius, radius,
                            0.f,
                            maxModDepthAngle, valAngle,
                            true
                        );
                        g.strokePath(modPath, strokeType);
                    }
                };

                col = Colours::c(CID::Interact);

                { // paint tick
                    const auto tickLine = LineF::fromStartAndAngle(centre, radius, valAngle);
                    const auto shortened = tickLine.withShortenedStart(radiusInner - thicc);
                    g.setColour(Colours::c(CID::Bg));
                    g.drawLine(shortened, thicc5);
                    g.setColour(col);
                    g.drawLine(shortened, thicc3);
                }
            };
    }

    void makeSlider(Knob& knob, bool showModulation)
    {
        knob.onPaint = [showModulation](Graphics& g, Knob& k)
            {
                const auto& vals = k.values;
                const auto valNorm = vals[Knob::Value];

                const auto enterExitPhase = k.callbacks[Knob::kEnterExitCB].phase;
                const auto downUpPhase = k.callbacks[Knob::kDownUpCB].phase;

                const auto thicc = k.utils.thicc;
                const auto thicc2 = thicc * 2.f;
                const auto thicc3 = thicc * 3.f;
                const auto thicc3Half = thicc3 * .5f;
                const auto thicc4 = thicc * 4.f;

                BoundsF bounds = k.getLocalBounds().toFloat().reduced(thicc3);
                const auto tickX = bounds.getX() + bounds.getWidth() * valNorm;

                { // paint lines
                    auto x = bounds.getX();
                    auto y = bounds.getY() + thicc2 + enterExitPhase * thicc;
                    auto w = bounds.getWidth();
                    auto h = thicc2 - enterExitPhase * thicc;

                    BoundsF line(x, y, w, h);

                    setCol(g, CID::Txt);
                    g.fillRoundedRectangle(line, thicc4);

                    if (downUpPhase != 0.f)
                    {
                        const auto dupFast = math::tanhApprox(downUpPhase * 2.f);
                        const auto centreX = line.getX() + line.getWidth() * .5f;

                        y = line.getBottom() + thicc;
                        h = line.getHeight() + thicc;
                        x = centreX + dupFast * (line.getX() - centreX);
                        w = 1.f + dupFast * (line.getWidth() - 1.f);
                        line = BoundsF(x, y, w, h);
                        g.fillRoundedRectangle(line, thicc);
                    }
                }

                if (showModulation)
                {
                    const auto modDepth = vals[Knob::ModDepth];

                    auto mdStart = tickX;
                    auto mdRange = bounds.getWidth() * modDepth;
                    if (mdRange < 0.f)
                    {
                        mdStart += mdRange;
                        mdRange = -mdRange;
                    }

                    auto x = mdStart;
                    auto y = bounds.getBottom() - thicc2;
                    auto w = mdRange;
                    auto h = thicc2;

                    const BoundsF modDepthArea(x, y, w, h);
                    setCol(g, CID::Mod);
                    g.fillRoundedRectangle(modDepthArea, thicc);

                    const auto valMod = vals[Knob::ValMod];

                    x = bounds.getX() + bounds.getWidth() * valMod - thicc;
                    y = bounds.getY() + thicc2;
                    w = thicc2;
                    h = bounds.getHeight();
                    const BoundsF modTick(x, y, w, h);
                    setCol(g, CID::Bg);
                    g.fillRoundedRectangle(modTick.expanded(thicc), thicc);
                    setCol(g, CID::Mod);
                    g.fillRoundedRectangle(modTick, thicc);
                };

                { // paint tick
                    const auto x = tickX - thicc3Half - enterExitPhase * thicc;
                    const auto y = bounds.getY();
                    const auto w = thicc3 + enterExitPhase * thicc2;
                    const auto h = bounds.getHeight() + thicc3;
                    const BoundsF tick(x, y, w, h);
                    const auto roundness = thicc2 + enterExitPhase * thicc;
                    if (downUpPhase != 0.f)
                    {
                        setCol(g, CID::Bg);
                        g.fillRoundedRectangle(tick.expanded(downUpPhase * thicc2), roundness);
                    }
                    setCol(g, CID::Interact);
                    g.fillRoundedRectangle(tick, roundness);
                }
            };
    }

    void makeKnob(PID pID, Knob& knob, bool showModulation)
    {
        makeParameter(pID, knob, true);
        makeKnob(knob, showModulation);
    }

    void makeSlider(PID pID, Knob& knob, bool showModulation)
    {
        makeParameter(pID, knob, false);
        makeSlider(knob, showModulation);
    }

    void locateAtKnob(ModDial& modDial, const Knob& knob)
    {
        const auto w0 = static_cast<float>(knob.getWidth());
        const auto w = w0 * .25f;
        const auto x = static_cast<float>(knob.getX()) + (w0 - w) * .5f;
        const auto h = static_cast<float>(knob.getHeight()) / Pi;
        const auto y = static_cast<float>(knob.getBottom()) - h;
        const auto nBounds = maxQuadIn(BoundsF(x, y, w, h)).toNearestInt();
        modDial.setBounds(nBounds);
    }

    void locateAtSlider(ModDial& modDial, const Knob& sliderHorizontal)
    {
        const auto thicc = modDial.utils.thicc;
        const auto x = static_cast<float>(sliderHorizontal.getRight());
        const auto y = static_cast<float>(sliderHorizontal.getY());
        const auto w = thicc * 8.f;
        const auto h = static_cast<float>(sliderHorizontal.getHeight());
        const BoundsF bounds(x, y, w, h);
        modDial.setBounds(maxQuadIn(bounds).toNearestInt());
    }
}