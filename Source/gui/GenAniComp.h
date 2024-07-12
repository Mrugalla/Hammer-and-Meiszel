#pragma once
#include "Comp.h"

namespace gui
{
	struct GenAniComp :
		public Comp
	{
		GenAniComp(Utils& u) :
			Comp(u, "If this thing makes the plugin lag, click on it to disable it globally for this user!"),
			img(),
			onResize([](){})
		{
			setOpaque(true);
		}

		void init()
		{
			const auto& user = utils.getProps();
			const auto active = user.getBoolValue("genanicomp");
			callbacks[0].active = active;
		}

		void paint(Graphics& g) override
		{
			g.drawImageAt(img, 0, 0);
		}

		void resized() override
		{
			if(img.isValid())
				img = img.rescaled(getWidth(), getHeight(), Graphics::lowResamplingQuality);
			else
				img = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
			fixStupidJUCEImageThingie(img);
			onResize();
		}

		void mouseUp(const Mouse& mouse) override
		{
			if (mouse.mouseWasDraggedSinceMouseDown())
				return;

			auto& active = callbacks[0].active;
			active = !active;
			auto& user = utils.getProps();
			user.setValue("genanicomp", active);
		}

	protected:
		Image img;
		std::function<void()> onResize;
	};

	struct GenAniGrowTrees :
		public GenAniComp
	{
		static constexpr float MutationLikelyness = .05f;
		static constexpr int MutationTime = 9;
		static constexpr float NewBranchMinY = .6f;

		GenAniGrowTrees(Utils& u) :
			GenAniComp(u),
			rand(),
			col(juce::uint8(rand.nextInt()), juce::uint8(rand.nextInt()), juce::uint8(rand.nextInt())),
			pos(),
			angle(0.f),
			alphaDownCount(0)
		{
			onResize = [&]()
			{
				const auto width = static_cast<float>(getWidth());
				const auto height = static_cast<float>(getHeight());
				pos.setXY(rand.nextFloat() * width, height);
			};

			const auto fps = cbFPS::k15;
			const auto speed = msToInc(AniLengthMs, fps);

			add(Callback([&, speed]()
			{
				auto& phase = callbacks[0].phase;
				phase += speed;
				if (phase >= 1.f)
					phase -= 1.f;

				Graphics g{ img };
				
				const auto width = static_cast<float>(getWidth());
				const auto height = static_cast<float>(getHeight());

				const auto length = 1.f + rand.nextFloat() * 2.f * utils.thicc;
				const auto line = LineF::fromStartAndAngle(pos, length, angle + PiQuart - rand.nextFloat() * PiHalf);
				const auto end = line.getEnd();

				g.setColour(col);
				const auto thicc = 1.f + rand.nextFloat() * 2.f * utils.thicc;
				g.drawLine(line, thicc);

				pos = end;
				
				const bool triggerBranch = rand.nextFloat() < MutationLikelyness;

				if (pos.y <= 0.f || triggerBranch)
					startNewBranch(width, height, 3.6f, .7f);

				++alphaDownCount;
				if (alphaDownCount >= 1 << MutationTime)
					mutate(width, height);

				repaint();
			}, 0, fps, false));

			init();
		}

	private:
		Random rand;
		juce::Colour col;
		PointF pos;
		float angle;
		int alphaDownCount;

		void startNewBranch(float width, float height, float gravity, float latchLikelyness) noexcept
		{
			static constexpr auto TimeOut = 1 << 10;
			const auto minY = NewBranchMinY * height;
			for (auto i = 0; i < TimeOut; ++i)
			{
				const auto x0 = width * rand.nextFloat();
				const auto rBias = math::tanhApprox(gravity * rand.nextFloat());
				const auto y0 = minY + (height - minY) * rBias * rBias;
				pos.setXY(x0, y0);
				auto pxl = img.getPixelAt(static_cast<int>(pos.x), static_cast<int>(pos.y));
				if (pxl.getPerceivedBrightness() > 1.f - latchLikelyness)
				{
					const auto hue = col.getHue() + .1f * (rand.nextFloat() - .5f);
					col = col.withHue(hue);
					const auto sat = col.getSaturationHSL() + .2f * (rand.nextFloat() - .5f);
					col = col.withSaturation(juce::jlimit(0.f, 1.f, sat));
					angle = juce::jlimit(-PiQuart, PiQuart, angle + .7f * (PiQuart - rand.nextFloat() * PiHalf));
					return;
				}
			}
		}

		void darken(float val) noexcept
		{
			for (auto y = 0; y < img.getHeight(); ++y)
				for (auto x = 0; x < img.getWidth(); ++x)
				{
					auto pxl = img.getPixelAt(x, y);
					pxl = pxl.withMultipliedBrightness(val);
					img.setPixelAt(x, y, pxl);
				}
		}

		void genNewCol(float mixOldNew, int tolerance) noexcept
		{
			angle = rand.nextFloat() * Pi - PiHalf;
			for (auto i = 0; i < tolerance; ++i)
			{
				auto nCol = Colour(static_cast<juce::uint8>(rand.nextInt()), static_cast<juce::uint8>(rand.nextInt()), static_cast<juce::uint8>(rand.nextInt()));
				nCol = nCol.withMultipliedSaturationHSL(1.f + rand.nextFloat() * 1.f);
				col = col.interpolatedWith(nCol, mixOldNew);
				auto brightness = col.getPerceivedBrightness();
				if (brightness > .5f)
					break;
			}
		}

		void mutate(float width, float height)
		{
			for(auto i = 0; i < 64; ++i)
				makePoroes(width, height);
			darken(.6f);
			genNewCol(.7f, 3);
			startNewBranch(width, height, 12.f, .7f);
			alphaDownCount = 0;
		}

		void makePoroes(float width, float height)
		{
			const auto darkenPos = PointF(rand.nextFloat() * width, rand.nextFloat() * height).toInt();
			Point mat[] =
			{
				{1, 0}, {0, 1}, {1, 1},
				{2, 0}, {0, 2}, {2, 2}, {2, 1}, {1, 2},
				{3, 1}, {3, 2}, {1, 3}, {2, 3}
			};
			for (auto j = 0; j < 12; ++j)
			{
				auto xx = darkenPos.x + mat[j].x;
				auto yy = darkenPos.y + mat[j].y;
				img.setPixelAt(xx, yy, Colour(0xff000000));
			}
		}
	};
}
