#pragma once
#include "Button.h"

namespace gui
{
	struct Knob :
		public Comp
	{
        static constexpr float SensitiveDrag = .15f;
        static constexpr float WheelSpeed = .018f;

		using Func = std::function<void()>;
		using OnDrag = std::function<void(const PointF&, const Mouse&)>;
		using OnMouse = std::function<void(const Mouse&)>;
		using OnPaint = std::function<void(Graphics&, Knob&)>;

        enum kCBs { kEnterExitCB, kDownUpCB, kUpdateParameterCB, kNumCallbacks };
        enum kVals { Value, ValMod, ModDepth, ModBias, NumValTypes };

        Knob(Utils&);

        void mouseEnter(const Mouse&) override;

        void mouseExit(const Mouse&) override;

        void mouseDown(const Mouse&) override;

        void mouseDrag(const Mouse&) override;

        void mouseUp(const Mouse&) override;

        void mouseWheelMove(const Mouse&, const MouseWheel&) override;

        void mouseDoubleClick(const Mouse&) override;

        void paint(Graphics& g) override;

        std::vector<PID> pIDs;
        std::vector<float> values;
        Func onEnter, onExit, onDown, onDoubleClick;
        OnDrag onDrag;
        OnMouse onUp, onWheel;
        OnPaint onPaint;
        PointF dragXY, lastPos;
        bool hidesCursor;
	};

    struct ModDial :
        public Knob
    {
        ModDial(Utils&);

        // pIDs, numPIDs
        void attach(PID*, int);

        // pID
        void attach(PID);

        void paint(Graphics&) override;

    private:
        std::vector<Param*> prms;
        Path path;
        bool showBias;
    public:
        bool verticalDrag;
    };

    // pIDs, knob, verticalDrag
    void makeParameters(const std::vector<PID>&, Knob&, bool = true);

    // pID, knob, verticalDrag
    void makeParameter(PID, Knob&, bool = true);

    // knob, showModulation
    void makeKnob(Knob&, bool = true);

    // knob, showModulation
    void makeSlider(Knob&, bool = true);

    // knob, showModulation
    void makeTextKnob(Knob&, bool = true);

    // pID, knob, showModulation
    void makeKnob(PID, Knob&, bool = true);

    // pID, knob, showModulation
    void makeSlider(PID, Knob&, bool = true);

    // pID, knob, showModulation
    void makeTextKnob(PID, Knob&, bool = true);

    // modDial, knob
    void locateAtKnob(ModDial&, const Knob&);

    // modDial, horizontalSlider
    void locateAtSlider(ModDial&, const Knob&);
}

/*
low stepsizes reveil that it picks the wrong points for the parameter values

wanna implement linear interpolation between points
*/