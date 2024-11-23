/*

modal material before i changed it from waveform to partials

#pragma once
#include "Comp.h"
#include "../audio/dsp/ModalFilter.h"

namespace gui
{
	struct ModalMaterialView :
		public Comp
	{
		using Material = dsp::modal::Material;

		static constexpr int StepSize = 2;
		static constexpr float StepSizeF = static_cast<float>(StepSize);
		using SampleBuffer = std::vector<float>;

		enum { kWaveformUpdatedCB, kNumCallbacks };

		ModalMaterialView(Utils& u, Material& _material) :
			Comp(u),
			material(_material),
			buffer(),
			drawBipolar(false)
		{
			add(Callback([&]()
			{
				if (material.updated.load())
				{
					update();
					repaint();
				}
			}, kWaveformUpdatedCB, cbFPS::k3_75, true));
		}

		void paint(Graphics& g) override
		{
			if (buffer.empty())
				return;

			const auto h = static_cast<float>(getHeight());
			paintWaveform(g, getColour(CID::Txt), h);
		}

		void resized() override
		{
			buffer.resize(getWidth() / StepSize, 0.f);
			update();
		}

		Material& material;
		SampleBuffer buffer;
		bool drawBipolar;
	private:
		void update()
		{
			updateSampleBuffer();
			material.updated.store(false);
		}

		void updateSampleBuffer()
		{
			auto numSamples = static_cast<double>(material.buffer.size());
			auto samples = material.buffer.data();

			for (auto i = 0; i < buffer.size(); ++i)
			{
				const auto iD = static_cast<double>(i);
				const auto iRatio = iD / static_cast<double>(buffer.size());
				const auto iSample = static_cast<int>(std::round(iRatio * numSamples));
				buffer[i] = static_cast<float>(samples[iSample]);
			}
		}

		void paintWaveform(Graphics& g, Colour c, float h)
		{
			g.setColour(c);
			BoundsF rect(0.f, 0.f, StepSizeF, 0.f);
			if (drawBipolar)
			{
				const auto centreY = h * .5f;
				for (auto i = 0; i < buffer.size(); ++i)
				{
					const auto x = rect.getX();
					const auto smpl = buffer[i];
					auto y = centreY - smpl * centreY;
					auto hgt = centreY - y;
					if (hgt < 0.f)
					{
						hgt = -hgt;
						y = centreY;
					}
					rect.setY(y);
					rect.setHeight(hgt);
					g.fillRect(rect);
					rect.setX(x + StepSizeF);
				}
			}
			else
			{
				for (auto i = 0; i < buffer.size(); ++i)
				{
					const auto x = rect.getX();
					const auto smpl = std::abs(buffer[i]);
					const auto y = h - smpl * h;
					const auto hgt = h - y;
					rect.setY(y);
					rect.setHeight(hgt);
					g.fillRect(rect);
					rect.setX(x + StepSizeF);
				}
			}
		}
	};
}

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

some code where a component follows the cursor:

const auto fpsToast = cbFPS::k30;
			add(Callback([&]()
			{
				if (!followingCursor || !isShowing())
					return;
				const auto screenBounds = utils.pluginTop.getScreenBounds().toFloat();
				const auto mouse = juce::Desktop::getInstance().getMainMouseSource();
				const auto mouseScreenPos = mouse.getScreenPosition();
				auto mousePos = mouseScreenPos - screenBounds.getPosition();
				if (mousePos.x < 0.f || mousePos.y < 0.f || mousePos.x > screenBounds.getWidth() || mousePos.y > screenBounds.getHeight())
					return setVisible(false);
				const auto width = static_cast<float>(getWidth());
				if (mousePos.x > width)
					mousePos.x -= width;
				else
					mousePos.x += width * .5f;
				setTopLeftPosition(mousePos.toInt());
			}, kCBFollowCursor, fpsToast, true));


/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

spirograph knob:

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
        if (mdState == MDState::Processing)
            pathModDepth.lineTo(curPt);

        if (drawArms)
            arms.drawArms(g, thicc);

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

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
*/

/*

//this was from an attempt to add allpass filters to the comb filter:

struct AllpassTransposedDirectFormII
		{
			AllpassTransposedDirectFormII() :
				a0(0.), a1(0.), a2(0.), b1(0.), b2(0.),
				z1(0.), z2(0.), fsInv(1.)
			{}

			void reset() noexcept
			{
				z1 = 0.;
				z2 = 0.;
			}

			void copyFrom(const AllpassTransposedDirectFormII& other) noexcept
			{
				a0 = other.a0;
				a1 = other.a1;
				a2 = other.a2;
				b1 = other.b1;
				b2 = other.b2;
			}

			// freqFc, qHz
			void updateParameters(double fc, double q) noexcept
			{
				const auto k = std::tan(math::Pi * fc);
				const auto kk = k * k;
				const auto kq = k / q;
				const auto norm = 1. / (1. + kq + kk);
				a0 = (1. - kq + kk) * norm;
				a1 = 2. * (kk - 1.) * norm;
				a2 = 1.;
				b1 = a1;
				b2 = a0;
			}

			double operator()(double x) noexcept
			{
				const auto y = a0 * x + z1;
				z1 = a1 * x + b1 * y + z2;
				z2 = a2 * x + b2 * y;
				return y;
			}

		private:
			double a0, a1, a2, b1, b2;
			double z1, z2, fsInv;
		};

		struct AllpassStereo
		{
			AllpassStereo() :
				filters()
			{
			}

			void reset() noexcept
			{
				for (auto& filter : filters)
					filter.reset();
			}

			void copyFrom(const AllpassStereo& other) noexcept
			{
				for (auto i = 0; i < filters.size(); ++i)
					filters[i].copyFrom(other.filters[i]);
			}

			// freqFc, qHz, ch
			void updateParameters(double fc, double q, int ch) noexcept
			{
				filters[ch].updateParameters(fc, q);
			}

			// freqFc, qHz
			void updateParameters(double fc, double q) noexcept
			{
				updateParameters(fc, q, 0);
				filters[1].copyFrom(filters[0]);
			}

			// smpl, ch
			double operator()(double x, int ch) noexcept
			{
				return filters[ch](x);
			}

		private:
			std::array<AllpassTransposedDirectFormII, 2> filters;
		};

		struct Disperser
		{
			Disperser(Parameters& _params) :
				params(_params),
				filters(),
				freqFc(-1.)
			{
			}

			void prepare()
			{
				for (auto& filter : filters)
					filter.reset();
				freqFc = -1.;
			}

			// freqFc, ch
			void setFreqFc(double _freqFc, int ch) noexcept
			{
				if (freqFc == _freqFc)
					return;
				freqFc = _freqFc;
				updateFreqFc(ch);
			}

			void updateFreqFc(int ch) noexcept
			{
				for (auto i = 0; i < SequenceSize; ++i)
				{
					const auto sequenceVal = params.getSequenceVal(i);
					auto& filter = filters[i];
					auto fc = sequenceVal * freqFc;
					if (fc >= .5)
						fc = .5 - std::numeric_limits<double>::epsilon();
					while (fc >= .5)
						fc -= .5;
					filter.updateParameters(fc, .01, ch);
				}
			}

			void updateAPReso(int numChannels) noexcept
			{
				const auto apReso = params.getAPReso();
				for (auto& apSlopeChannels : filters)
				{
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						//auto& filter = filters[ch];
						//filter.updateParameters(100., apReso);
					}
				}
			}

			double operator()(double smpl, int ch) noexcept
			{
				auto y = smpl;
				for (auto i = 0; i < SequenceSize; ++i)
				{
					auto& filter = filters[i];
					y = filter(y, ch);
				}
				return y;
			}
		protected:
			Parameters& params;
			std::array<AllpassStereo, SequenceSize> filters;
			double freqFc;
		};

*/