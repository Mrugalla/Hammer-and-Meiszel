#pragma once
#include "Button.h"

namespace gui
{
	struct Knob :
		public Comp
	{
        static constexpr float SensitiveDrag = .2f;
        static constexpr float WheelSpeed = .02f;

		using Func = std::function<void()>;
		using OnDrag = std::function<void(const PointF&, const Mouse&)>;
		using OnMouse = std::function<void(const Mouse&)>;

        enum { kEnterExitCB, kDownUpCB, kNumCallbacks };

        Knob(Utils& u) :
            Comp(u),
            onEnter([]() {}), onExit([]() {}), onDown([]() {}), onDoubleClick([]() {}),
            onDrag([](const PointF&, const Mouse&) {}),
            onUp([](const Mouse&) {}), onWheel([](const Mouse&) {}),
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

        void mouseEnter(const Mouse& mouse) override
        {
            Comp::mouseEnter(mouse);
            callbacks[kEnterExitCB].start(0.f);
            onEnter();
        }

        void mouseExit(const Mouse& mouse) override
        {
            Comp::mouseExit(mouse);
			onExit();
        }

        void mouseDown(const Mouse& mouse) override
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

        void mouseDrag(const Mouse& mouse) override
        {
            if (hidesCursor)
                hideCursor();

            const auto nPosition = getPosition().toFloat();
            const auto posShift = nPosition - lastPos;
            lastPos = nPosition;

            dragXY -= posShift;
            const auto shiftDown = mouse.mods.isShiftDown();
            const auto dragSpeed = shiftDown ? SensitiveDrag : 1.f;
            const auto dragOffset = (mouse.position - dragXY) * dragSpeed;
            onDrag(dragOffset, mouse);
            dragXY = mouse.position;
        }

        void mouseUp(const Mouse& mouse) override
        {
            onUp(mouse);

            if (hidesCursor && mouse.mouseWasDraggedSinceMouseDown())
                showCursor(*this);
        }

        void mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel) override
        {
            if (mouse.mods.isAnyMouseButtonDown())
                return;

            const bool reversed = wheel.isReversed ? -1.f : 1.f;
            const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
            if (isTrackPad)
                dragXY.setXY
                (
                    reversed * wheel.deltaX,
                    reversed * wheel.deltaY
                );
            else
            {
                const auto deltaXPos = wheel.deltaX > 0.f ? 1.f : -1.f;
                const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
                dragXY.setXY
                (
                    reversed * WheelSpeed * deltaXPos,
                    reversed * WheelSpeed * deltaYPos
                );
            }

            if (mouse.mods.isShiftDown())
                dragXY *= SensitiveDrag;

            onWheel(mouse);
        }

        void mouseDoubleClick(const Mouse& mouse) override
        {
            Comp::mouseDoubleClick(mouse);
            onDoubleClick();
        }

        Func onEnter, onExit, onDown, onDoubleClick;
        OnDrag onDrag;
        OnMouse onUp, onWheel;
        PointF dragXY, lastPos;
        bool hidesCursor;
	};

    struct ModDial :
        public Knob
    {
        ModDial(Utils& u) :
            Knob(u),
            prms(),
            biasBounds(),
            path(),
            wOff(0.f)
        {
            onDrag = [&](const PointF& dragOffset, const Mouse& mouse)
            {
                const auto speed = 1.f / utils.getDragSpeed();
                const auto dragXY = dragOffset * speed;

                if (mouse.mods.isRightButtonDown())
                    for (auto prm : prms)
                        prm->setModBias(prm->getModBias() - dragXY.y);
                else
                    for (auto prm : prms)
                        prm->setModDepth(prm->getModDepth() - dragXY.y);
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
                    const auto& range = prm->range;
                    const auto interval = range.interval;

                    if (interval > 0.f)
                    {
                        const auto nStep = interval / range.getRange().getLength();
                        dragXY.setY(dragXY.y > 0.f ? nStep : -nStep);
                    }
                    
                    prm->setModDepth(prm->getModDepth() + dragXY.y);
                }
            };
        }

        void attachParameters(PID* pIDs, int numPIDs)
        {
            prms.clear();
            prms.reserve(numPIDs);
            for (auto i = 0; i < numPIDs; ++i)
                prms.emplace_back(&utils.getParam(pIDs[i]));
        }

        void resized() override
        {
            const auto w = static_cast<float>(getWidth());
            wOff = w * .2f;
            biasBounds = getLocalBounds().toFloat().reduced(wOff);
        }

        void paint(Graphics& g) override
        {
            const auto enterExitPhase = callbacks[kEnterExitCB].phase;
            
            g.setColour(getColour(CID::Mod));
            g.fillEllipse(getLocalBounds().toFloat());

            path.clear();
            const auto& mainParam = *prms[0];
            const auto bias = mainParam.getModBias();

            const auto left = biasBounds.getX();
            const auto btm = biasBounds.getBottom();
            const auto right = biasBounds.getRight();
            const auto top = biasBounds.getY();
            const auto width = biasBounds.getWidth();
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

        std::vector<Param*> prms;
        BoundsF biasBounds;
        Path path;
        float wOff;
    };

    struct KnobParam :
        public Knob
    {
        enum CBTypes { kUpdateParameterCB };
        enum ValTypes { Value, ValMod, ModDepth, ModBias, NumValTypes };

        using OnLayout = std::function<void(Layout&)>;
        using OnResize = std::function<void(KnobParam&)>;
        using OnPaint = std::function<void(KnobParam&, Graphics&)>;

        struct Painter
        {
            Painter() :
                onLayout([](Layout&) {}),
                onResize([](KnobParam&) {}),
                onPaint([](KnobParam&, Graphics&) {})
            {
            }

            OnLayout onLayout;
            OnResize onResize;
            OnPaint onPaint;
        };

        KnobParam(Utils& u) :
            Knob(u),
            painter(nullptr),
            values{ 0.f, 0.f, 0.f, 0.f },
            prms(),
            modDial(u),
            lockButton(u),
            label(u)
        {
		    addAndMakeVisible(lockButton);
            makeTextButton(lockButton, "L", "Click here to lock this parameter.", CID::Interact);
            lockButton.onClick = [&](const Mouse&)
			{
                for (auto prm : prms)
                    prm->switchLock();
			};
        }

        void attachParameters(const String& name, PID* pIDs, int numPIDs, Painter* p)
        {
            getParentComponent()->addAndMakeVisible(label);

            prms.clear();
            prms.reserve(numPIDs);
            for (auto i = 0; i < numPIDs; ++i)
                prms.emplace_back(&utils.getParam(pIDs[i]));

            setTooltip(toTooltip(pIDs[0]));
            setName(name);

            makeTextLabel(label, name, font::dosisMedium(), Just::centredBottom, CID::Txt, tooltip);
            auto& mainParam = utils.getParam(pIDs[0]);

            const auto drawValToLabelFunc = [&, &prm = mainParam]()
            {
                label.setText(prm.getCurrentValueAsText());
                label.setMaxHeight();
                label.repaint();
            };

            onEnter = [drawValToLabelFunc]()
            {
                drawValToLabelFunc();
            };

            onExit = [&]()
            {
                label.setText(getName());
                label.setMaxHeight();
                label.repaint();
            };

            onDown = [&, drawValToLabelFunc]()
            {
                for (auto prm : prms)
                    if (!prm->isInGesture())
                        prm->beginGesture();

                drawValToLabelFunc();
            };

            onDrag = [&, drawValToLabelFunc](const PointF& dragOffset, const Mouse&)
            {
                const auto speed = 1.f / utils.getDragSpeed();
                const auto dragVal = -dragOffset.y * speed;

                for (auto prm : prms)
                {
                    const auto newValue = juce::jlimit(0.f, 1.f, prm->getValue() + dragVal);
                    prm->setValueFromEditor(newValue);
                }

                drawValToLabelFunc();
            };

            onUp = [&, drawValToLabelFunc](const Mouse& mouse)
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isAltDown())
                        for (auto prm : prms)
                            prm->setValueFromEditor(prm->getDefaultValue());

                for (auto prm : prms)
                    prm->endGesture();

                drawValToLabelFunc();
            };

            onWheel = [&, drawValToLabelFunc](const Mouse&)
            {
                for (auto prm : prms)
                {
                    const auto& range = prm->range;
                    const auto interval = range.interval;

                    if (interval > 0.f)
                    {
                        const auto nStep = interval / range.getRange().getLength();
                        dragXY.setY(dragXY.y > 0.f ? nStep : -nStep);
                        auto newValue = juce::jlimit(0.f, 1.f, prm->getValue() + dragXY.y);
                        newValue = range.convertTo0to1(range.snapToLegalValue(range.convertFrom0to1(newValue)));
                        prm->setValueWithGesture(newValue);
                    }
                    else
                    {
                        const auto newValue = juce::jlimit(0.f, 1.f, prm->getValue() + dragXY.y);
                        prm->setValueWithGesture(newValue);
                    }
                }

                drawValToLabelFunc();
            };

            onDoubleClick = [&, drawValToLabelFunc]()
            {
                for (auto prm : prms)
                    if (!prm->isInGesture())
                        prm->setValueWithGesture(prm->getDefaultValue());

                drawValToLabelFunc();
            };

            bool isMacro = pIDs[0] == PID::Macro;
            if (!isMacro)
            {
                addAndMakeVisible(modDial);
                modDial.attachParameters(pIDs, numPIDs);
            }

            add(Callback([&, &prm = mainParam]()
            {
                bool shallRepaint = false;

                setLocked(prm.isLocked());

                const auto vn = prm.getValue();
                const auto md = prm.getModDepth();
                const auto vm = prm.getValMod();
                const auto mb = prm.getModBias();

                auto& vals = values;

                if (vals[Value] != vn || vals[ModDepth] != md || vals[ValMod] != vm || vals[ModBias] != mb)
                {
                    vals[Value] = vn;
                    vals[ModDepth] = md;
                    vals[ValMod] = vm;
                    vals[ModBias] = mb;
                    shallRepaint = true;
                }

                if (shallRepaint)
                    repaint();
            }, kUpdateParameterCB, cbFPS::k60, true));

            painter = p;
            painter->onLayout(layout);
        }

        void attachParameter(const String& name, PID pID, Painter* p)
		{
			attachParameters(name, &pID, 1, p);
		}

        void resized() override
        {
            layout.resized(getLocalBounds());
            painter->onResize(*this);
        }

        void paint(Graphics& g) override
        {
            painter->onPaint(*this, g);
        }

        Painter* painter;
        std::array<float, NumValTypes> values;
        std::vector<Param*> prms;
        ModDial modDial;
        Button lockButton;
        Label label;
    };

    struct KnobPainterBasic :
        public KnobParam::Painter
    {
        KnobPainterBasic(bool isMacro) :
            knobBounds(),
            thicc2(0.f), thicc3(0.f), thicc4(0.f), thicc5(0.f)
        {
            onLayout = [](Layout& layout)
            {
                layout.init
                (
                    { 1, 1, 1 },
                    { 13, 5, 5 }
                );
            };

            onResize = [&](KnobParam& k)
            {
                const auto thicc = k.utils.thicc;
                thicc2 = thicc * 2.f;
                thicc3 = thicc * 3.f;
                thicc4 = thicc * 4.f;
                thicc5 = thicc * 5.f;

                knobBounds = k.layout(0, 0, 3, 2, true).reduced(thicc3);

                k.layout.place(k.label, 0, 2, 3, 1, false);

                if (k.modDial.isVisible())
                    k.layout.place(k.modDial, 1, 1, 1, 1, true);
                k.layout.place(k.lockButton, 1.5f, 1.5f, 1.5f, 1.5f, true);
			};

            const auto angleWidth = PiQuart * 3.f;
            const auto angleRange = angleWidth * 2.f;

            onPaint = [&, angleWidth, angleRange, isMacro](KnobParam& k, Graphics& g)
            {
                const auto& vals = k.values;
                const auto thicc = k.utils.thicc;

                const auto enterExitPhase = k.callbacks[KnobParam::kEnterExitCB].phase;
                const auto downUpPhase = k.callbacks[KnobParam::kDownUpCB].phase;

                const auto radius = knobBounds.getWidth() * .5f;
                const auto radiusInner = radius * (.7f + enterExitPhase * .1f);
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
                        -angleWidth, angleWidth,
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
                        -angleWidth, angleWidth,
                        true
                    );
                    Stroke stroke2 = strokeType;
                    stroke2.setStrokeThickness(radDif);
                    g.strokePath(arcInline, stroke2);
                };

                const auto valNormAngle = vals[KnobParam::Value] * angleRange;
                const auto valAngle = -angleWidth + valNormAngle;
                const auto radiusExt = radius + thicc;

                // paint modulation
                if (!isMacro)
                {
                    const auto valModAngle = vals[KnobParam::ValMod] * angleRange;
                    const auto modAngle = -angleWidth + valModAngle;
                    const auto modTick = LineF::fromStartAndAngle(centre, radiusExt, modAngle);
                    const auto shortenedModTick = modTick.withShortenedStart(radiusInner - thicc);

                    g.setColour(Colours::c(CID::Bg));
                    g.drawLine(shortenedModTick, thicc4);

                    const auto maxModDepthAngle = juce::jlimit(-angleWidth, angleWidth, valNormAngle + vals[KnobParam::ModDepth] * angleRange - angleWidth);

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

    protected:
        BoundsF knobBounds;
        float thicc2, thicc3, thicc4, thicc5;
    };

    struct KnobPainterSpirograph :
        public KnobParam::Painter
    {
        enum class MDState { Waiting, Processing, Finished};

        struct Arm
        {
            LineF line;
            float angle, inc, lengthRel, length;

            void makeLine(const PointF centre)
            {
                line = LineF::fromStartAndAngle(centre, length, angle);
            }

            PointF getEnd() const noexcept
			{
				return line.getEnd();
			}

            void incAngle() noexcept
            {
                angle += inc;
            }

            void prepare(float aF, float angleMain, float numRings, float rad) noexcept
            {
                inc = angleMain + aF * numRings * angleMain;
                length = rad * lengthRel;
            }
        };

        struct Arms
        {
            Arms(float _startAngle, int numArms) :
                arms(),
                startAngle(_startAngle * Tau)
            {
                arms.resize(numArms);
            }

            void makeLengthRels(float lengthBias) noexcept
            {
                const auto numArmsF = static_cast<float>(arms.size());
                for (auto a = 0; a < arms.size(); ++a)
                {
                    const auto aF = static_cast<float>(a);
                    const auto aRatio = aF / numArmsF;
                    arms[a].lengthRel = Param::biased(0.f, 1.f, lengthBias, aRatio);
                }

                for (auto a = 0; a < arms.size() - 1; ++a)
                    arms[a].lengthRel = arms[a + 1].lengthRel - arms[a].lengthRel;
                arms.back().lengthRel = 1.f - arms.back().lengthRel;
            }

            void makeArms(const PointF centre)
            {
                arms[0].makeLine(centre);
                for (auto a = 1; a < arms.size(); ++a)
                    arms[a].makeLine(arms[a - 1].getEnd());
            }

            PointF getEnd() const noexcept
            {
                return arms.back().getEnd();
            }

            void drawArms(Graphics& g, float thicc)
            {
                for (auto& arm : arms)
                    g.drawLine(arm.line, thicc);
            }

            void incAngles() noexcept
            {
                for (auto& arm : arms)
                    arm.incAngle();
            }

            void resetAngles() noexcept
			{
				for (auto& arm : arms)
					arm.angle = startAngle;
			}

            void prepareArms(float angleMain, float numRings, float rad) noexcept
            {
                resetAngles();

                for (auto a = 0; a < arms.size(); ++a)
                {
                    auto& arm = arms[a];
                    const auto aF = static_cast<float>(a);
                    arm.prepare(aF, angleMain, numRings, rad);
                }
            }

            PointF getPointOfStep(const PointF centre, int stepIdx) noexcept
            {
                resetAngles();

                for (auto i = 0; i < stepIdx; ++i)
                {
                    incAngles();
                    makeArms(centre);
                } 

                return getEnd();
            }

            std::vector<Arm> arms;
            float startAngle;
        };

        struct Knot
        {
            void saveVal(float val, float numStepsF) noexcept
            {
                step = val * numStepsF;
                doLerp();
            }

            void saveModDepth(float modDepth, float numStepsF, float mainValStep) noexcept
            {
                step = juce::jlimit(0.f, numStepsF, mainValStep + modDepth * numStepsF);
                doLerp();
            }

            void makeBounds(float knotSize) noexcept
            {
                const auto knotSize2 = knotSize * 2.f;
                bounds.setBounds(x - knotSize, y - knotSize, knotSize2, knotSize2);
            }

            bool savePoint(const LineF& line, int idx)
            {
                if (iCeil != idx)
                    return false;

                const auto len = line.getLength();
                const auto pt = line.getPointAlongLine(frac * len);
                x = pt.x;
                y = pt.y;

                return true;
            }

            void processModDepthVal(Path& modDepth, const LineF& line, int idx,
                MDState& mdState, bool modDepthPositive)
            {
                isOnValue = savePoint(line, idx);
                if (isOnValue)
                {
                    if (modDepthPositive)
                    {
                        modDepth.startNewSubPath(x, y);
                        mdState = MDState::Processing;
                    }
                    else
                    {
                        if (mdState == MDState::Waiting)
                            return;
                        modDepth.lineTo(x, y);
                        mdState = MDState::Finished;
                    }
                }
            }

            bool processModDepthMod(Path& modDepth, const LineF& line, int idx,
                MDState& mdState, bool modDepthPositive)
            {
                isOnValue = savePoint(line, idx);
                if (isOnValue)
                {
                    if (modDepthPositive)
                    {
                        modDepth.lineTo(x, y);
                        mdState = MDState::Finished;
                    }
                    else
                    {
                        switch (mdState)
                        {
                        case MDState::Waiting:
                            modDepth.startNewSubPath(x, y);
                            mdState = MDState::Processing;
                            return true;
                        }
                    }
                }
                return false;
            }

            float x, y, step, frac, vFloor;
            int iFloor, iCeil;
            BoundsF bounds;
            bool isOnValue;

            void doLerp() noexcept
            {
                vFloor = std::floor(step);
                iFloor = static_cast<int>(vFloor);
                iCeil = iFloor + 1;
                frac = step - vFloor;
            }
        };

        KnobPainterSpirograph(int numArms = 2,
            float startAngle = .5f, float angleLength = 1.f,
            float lengthBias = .4f, float lengthBiasAni = .3f,
            float armsAlpha = .4f, float armsAlphaAni = -.2f,
            int numRings = 1, int numSteps = 7) :
            bounds(),
            spirograph(),
            pathModDepth(),
            arms(startAngle, numArms),
            knots()
        {
            onLayout = [](Layout& layout)
            {
                layout.init
                (
                    { 1, 1 },
                    { 5, 1 }
                );
            };

            onResize = [&](KnobParam& k)
            {
                const auto thicc = k.utils.thicc;
                bounds = k.layout(0, 0, 2, 1, true).reduced(thicc);

                if (k.modDial.isVisible())
                    k.layout.place(k.modDial, 0, 1, 1, 1, true);
                k.layout.place(k.lockButton, 1, 1, 1, 1, true);
            };

            onPaint = [&, lengthBias, lengthBiasAni, numArms, angleLength, armsAlpha, armsAlphaAni, numRings, numSteps](KnobParam& k, Graphics& g)
            {
                spirograph.clear();
                pathModDepth.clear();

                const auto thicc = k.utils.thicc;

                const auto& vals = k.values;
                const auto enterExitPhase = k.callbacks[KnobParam::kEnterExitCB].phase;
                const auto downUpPhase = k.callbacks[KnobParam::kDownUpCB].phase;

                const auto width = bounds.getWidth();
                const auto rad = width * .5f;
                PointF centre
                (
                    rad + bounds.getX(),
                    rad + bounds.getY()
                );
                const auto numStepsF = static_cast<float>(numSteps);
                const auto numStepsInv = 1.f / numStepsF;
                const auto angleMain = angleLength * Tau * numStepsInv;

                auto biasWithAni = lengthBias + lengthBiasAni * enterExitPhase + lengthBiasAni * downUpPhase * .5f;
                if(biasWithAni >= 1.f)
                    --biasWithAni;
                arms.makeLengthRels(biasWithAni);
                arms.prepareArms(angleMain, static_cast<float>(numRings), rad);
                const bool drawArms = armsAlpha != 0.f;

                auto& knotVal = knots[KnobParam::Value];
                auto& knotValMod = knots[KnobParam::ValMod];
                auto& knotModDepth = knots[KnobParam::ModDepth];

                knotVal.saveVal(vals[KnobParam::Value], numStepsF);
                knotValMod.saveVal(vals[KnobParam::ValMod], numStepsF);
                knotModDepth.saveModDepth(vals[KnobParam::ModDepth], numStepsF, knotVal.step);
                const bool modDepthPositive = vals[KnobParam::ModDepth] > 0.f;
                auto mdState = MDState::Waiting;

                arms.makeArms(centre);
                auto curPt = arms.getEnd();
                spirograph.startNewSubPath(curPt);
                if (drawArms)
                {
                    const auto alphaWithAni = juce::jlimit(0.f, 1.f, armsAlpha + armsAlphaAni * downUpPhase);
                    g.setColour(getColour(CID::Txt).withAlpha(alphaWithAni));
                    arms.drawArms(g, thicc);
                }

                /*
                if (knotVal.step == 0.f)
                {
                    knotVal.x = curPt.x;
                    knotVal.y = curPt.y;

                    if (modDepthPositive)
                    {
                        pathModDepth.startNewSubPath(curPt);
                        mdState = MDState::Processing;
                    }
                }
                if (knotValMod.step == 0.f)
				{
					knotValMod.x = curPt.x;
					knotValMod.y = curPt.y;

                    if (!modDepthPositive)
                    {
                        pathModDepth.startNewSubPath(curPt);
                        mdState = MDState::Processing;
                    }
				}
                */

                for (auto i = 1; i < numSteps; ++i)
                {
                    const auto lastPt = arms.getEnd();
                    arms.incAngles();
                    arms.makeArms(centre);
                    curPt = arms.getEnd();

                    LineF curLine(lastPt, curPt);
                    knotVal.processModDepthVal(pathModDepth, curLine, i, mdState, modDepthPositive);
                    if (knotValMod.processModDepthMod(pathModDepth, curLine, i, mdState, modDepthPositive))
                    {
                        if (knotVal.isOnValue)
                        {
                            pathModDepth.lineTo(knotVal.x, knotVal.y);
                            mdState = MDState::Finished;
                        }
                    }
                    
                    spirograph.lineTo(curPt);
                    if(mdState == MDState::Processing)
						pathModDepth.lineTo(curPt);

                    if (drawArms)
                        arms.drawArms(g, thicc);

                    /*
                    if (i == mainValStepCeil)
                    {
                        //const LineF armsLine(lastPt, curPt);
                        //const PointF interpolatedPt = armsLine.getPointAlongLine(mainValStepFrac * armsLine.getLength());
                        
                        valPoints[KnobParam::Value] = curPt;
                        if (modDepthPositive)
                        {
                            pathModDepth.startNewSubPath(curPt);
                            shallDrawModDepth = true;
                        }
                        else
                            shallDrawModDepth = false;
                    }

                    if (i == (int)valModStep)
                        valPoints[KnobParam::ValMod] = curPt;

                    if (i == (int)modDepthStep)
                    {
                        valPoints[KnobParam::ModDepth] = curPt;
                        if (modDepthPositive)
                            shallDrawModDepth = false;
                        else
                        {
                            pathModDepth.startNewSubPath(curPt);
                            shallDrawModDepth = true;
                        }
                    }
                    */
                }
                const auto lastPt = arms.getEnd();
                spirograph.closeSubPath();
                curPt = spirograph.getCurrentPosition();

                knotVal.processModDepthVal(pathModDepth, LineF(lastPt, curPt), numSteps, mdState, modDepthPositive);
                if (knotValMod.processModDepthMod(pathModDepth, LineF(lastPt, curPt), numSteps, mdState, modDepthPositive))
                {
                    pathModDepth.lineTo(knotVal.x, knotVal.y);
                    mdState = MDState::Finished;
                }

                if (mdState == MDState::Processing)
                    pathModDepth.lineTo(curPt);

                Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::butt);
                g.setColour(getColour(CID::Txt));
                g.strokePath(spirograph, stroke);

                const auto knotSize = thicc * 1.5f + downUpPhase * thicc * 1.5f;

                knotVal.makeBounds(knotSize);
                knotValMod.makeBounds(knotSize);
                
                g.setColour(getColour(CID::Mod));
                stroke.setStrokeThickness(knotSize);
                g.strokePath(pathModDepth, stroke);
                stroke.setStrokeThickness(thicc);

                g.setColour(getColour(CID::Bg));
                g.fillEllipse(knotVal.bounds);
                g.fillEllipse(knotValMod.bounds);

                g.setColour(getColour(CID::Interact));
                g.drawEllipse(knotVal.bounds, thicc);
                g.setColour(getColour(CID::Mod));
                g.drawEllipse(knotValMod.bounds, thicc);
            };
        }

        BoundsF bounds;
        Path spirograph, pathModDepth;
        Arms arms;
        std::array<Knot, KnobParam::NumValTypes> knots;
    };

    struct KnobPainterIdk :
        KnobParam::Painter
    {
        KnobPainterIdk()
        {
            onLayout = [](Layout& layout)
            {
                layout.init
                (
                    { 1, 1, 1 },
                    { 13, 5 }
                );
            };

            onResize = [&](KnobParam& k)
            {
                const auto thicc = k.utils.thicc;
                knobBounds = k.layout(0, 0, 3, 1, true).reduced(thicc);

                //k.layout.place(k.label, 0, 2, 3, 1, false);
                //k.label.setMaxHeight();

                if (k.modDial.isVisible())
                    k.layout.place(k.modDial, 1, 1, 1, 1, true);
                k.layout.place(k.lockButton, 1, 1, 1, 1, true);
            };

            onPaint = [&](KnobParam& k, Graphics& g)
            {
                const auto thicc = k.utils.thicc;

                g.setColour(getColour(CID::Interact));

                const auto& vals = k.values;
                const auto val = vals[KnobParam::ValTypes::Value];

                auto& range = k.prms[0]->range;
                auto interval = range.interval;
                float numCircles = 1.f + 15.f * val;
                if (interval != 0.f)
                {
                    numCircles = k.prms[0]->getValueDenorm();
                }
                for (auto i = 0.f; i < numCircles; ++i)
                {
                    auto ratio = i / numCircles;
                    auto delta = thicc + knobBounds.getWidth() * ratio * val;

                    g.drawEllipse(knobBounds.reduced(delta), thicc + (thicc * val));
                }
            };
        }

        BoundsF knobBounds;
    };
}

/*
todo:

low stepsizes reveil that it picks the wrong points for the parameter values

wanna implement linear interpolation between points


*/