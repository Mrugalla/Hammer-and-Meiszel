#pragma once
#include "Comp.h"

namespace gui
{
	struct GenAniComp :
		public Comp
	{
		GenAniComp(Utils& u, String&& _tooltip) :
			Comp(u, _tooltip),
			img(),
			onResize([](){}),
			active(utils.getProps().getBoolValue("genaniactive", true))
		{
			setOpaque(true);
		}

		~GenAniComp()
		{
			saveImage();
		}

		void init()
		{
			callbacks[0].active = active;
		}

		void paint(Graphics& g) override
		{
			g.drawImageAt(img, 0, 0);
		}

		bool loadImage()
		{
			auto& user = utils.getProps();
			const auto& file = user.getFile();
			const auto directory = file.getParentDirectory();
			const auto path = directory.getFullPathName() + File::getSeparatorString() + "genani.png";
			const auto findFiles = File::TypesOfFileToFind::findFiles;
			const auto wildCard = "*.png";
			for (auto f : directory.findChildFiles(findFiles, true, wildCard))
			{
				if (f.getFileName() == "genani.png")
				{
					const auto nImg = juce::ImageFileFormat::loadFrom(f);
					if (nImg.isValid())
					{
						img = nImg;
						return true;
					}
				}
			}
			return false;
		}

		void saveImage()
		{
			auto& user = utils.getProps();
			const auto directory = user.getFile().getParentDirectory();
			auto file = directory.getChildFile("genani.png");
			if (file.existsAsFile())
				file.deleteFile();
			juce::FileOutputStream stream(file);
			juce::PNGImageFormat pngWriter;
			pngWriter.writeImageToStream(img, stream);
		}

		void mouseUp(const Mouse& mouse) override
		{
			if (mouse.mouseWasDraggedSinceMouseDown())
				return;
			active = !active;
			callbacks[0].active = active;
			if (!active)
				saveImage();
			auto& user = utils.getProps();
			user.setValue("genaniactive", active);
		}

		void resized() override
		{
			if (img.isValid())
				img = img.rescaled(getWidth(), getHeight(), Graphics::lowResamplingQuality);
			else
			{
				if(!loadImage())
					img = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
			}
			fixStupidJUCEImageThingie(img);
			onResize();
		}
	protected:
		Image img;
		std::function<void()> onResize;
		bool active;

		
	};

	struct GenAniGrowTrees :
		public GenAniComp
	{
		static constexpr float MutationLikelyness = .05f;
		static constexpr int MutationTime = 9;
		static constexpr float NewBranchMinY = .6f;

		GenAniGrowTrees(Utils& u) :
			GenAniComp(u, "Get mesmerized by these beautiful growing branches!"),
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

			const auto fps = cbFPS::k7_5;
			const auto speed = msToInc(AniLengthMs, fps);

			add(Callback([&, speed]()
			{
				if (!img.isValid())
					return;

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

		void darken(float valTop, float valBtm) noexcept
		{
			Colour black(0xff000000);

			const auto h = static_cast<float>(img.getHeight());
			const auto hInv = 1.f / h;
			const auto valRange = valBtm - valTop;
			for (auto y = 0; y < img.getHeight(); ++y)
			{
				const auto yF = static_cast<float>(y);
				const auto yR = yF * hInv;
				const auto val = valTop + yR * valRange;
				for (auto x = 0; x < img.getWidth(); ++x)
				{
					auto pxl = img.getPixelAt(x, y);
					pxl = pxl.interpolatedWith(black, val);
					img.setPixelAt(x, y, pxl);
				}
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
			darken(1.f, .4f);
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
