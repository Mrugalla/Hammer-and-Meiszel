#include "GenAniComp.h"

namespace gui
{
	// GenAniComp

	GenAniComp::GenAniComp(Utils& u, String&& _tooltip) :
		Comp(u, _tooltip),
		img(),
		onResize([]() {}),
		active(utils.getProps().getBoolValue("genaniactive", true)),
		mode(utils.getProps().getIntValue("genanimode", 0)),
		numModes(1)
	{
		setOpaque(true);
	}

	GenAniComp::~GenAniComp()
	{
		saveImage();
	}

	void GenAniComp::init()
	{
		callbacks[0].active = active;
	}

	void GenAniComp::paint(Graphics& g)
	{
		g.drawImageAt(img, 0, 0);
	}

	bool GenAniComp::loadImage()
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

	void GenAniComp::saveImage()
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

	void GenAniComp::mouseUp(const Mouse& mouse)
	{
		if (mouse.mouseWasDraggedSinceMouseDown())
			return;
		if (mouse.mods.isCtrlDown())
		{
			mode = (mode + 1) % numModes;
			utils.getProps().setValue("genanimode", mode);
			return;
		}
		active = !active;
		callbacks[0].active = active;
		if (!active)
			saveImage();
		auto& user = utils.getProps();
		user.setValue("genaniactive", active);
	}

	void GenAniComp::resized()
	{
		if (img.isValid())
			img = img.rescaled(getWidth(), getHeight(), Graphics::lowResamplingQuality);
		else
		{
			if (!loadImage())
				img = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
		}
		fixStupidJUCEImageThingie(img);
		onResize();
	}

	// H&M Ani

	GenAniGrowTrees::GenAniGrowTrees(Utils& u) :
		GenAniComp(u, "Get mesmerized by this procedural stuff! (CTRL+Click to switch mode)"),
		rand(),
		col(juce::uint8(rand.nextInt()), juce::uint8(rand.nextInt()), juce::uint8(rand.nextInt())),
		pos(),
		angle(0.f),
		alphaDownCount(0)
	{
		numModes = kNumModes;

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

				Graphics g{ img };

				switch (mode)
				{
				case kTree:
					treeProcess(g);
					break;
				case kTech:
					techProcess(g);
					break;
				default:
					treeProcess(g);
					break;
				}
				repaint();
			}, 0, fps, false));

		init();
	}

	void GenAniGrowTrees::treeProcess(Graphics& g)
	{
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
	}

	void GenAniGrowTrees::techProcess(Graphics& g)
	{
		const auto width = getWidth();
		const auto height = getHeight();

		const auto flipx = [width](int x)
			{
				if (x < 0)
					return x + width;
				if (x >= width)
					return x - width;
				return x;
			};
		const auto flipy = [height](int y)
			{
				if (y < 0)
					return y + height;
				if (y >= height)
					return y - height;
				return y;
			};

		const auto flipxF = [w = static_cast<float>(width)](float x)
			{
				if (x < 0.f)
					return x + w;
				if (x >= w)
					return x - w;
				return x;
			};
		const auto flipyF = [h = static_cast<float>(height)](float y)
			{
				if (y < 0.f)
					return y + h;
				if (y >= h)
					return y - h;
				return y;
			};

		int numSpotsFound = 0;
		const auto numSteps = 64;
		for (auto i = 0; i < numSteps; ++i)
		{
			const auto x = rand.nextInt(width);
			const auto y = rand.nextInt(height);
			const auto pxl = img.getPixelAt(x, y);
			if (pxl.getBrightness() > .1f)
			{
				++numSpotsFound;
				g.setColour(pxl.withRotatedHue(rand.nextFloat() * .01f));
				const auto randSquared = rand.nextFloat();
				const auto rad = 1.f + randSquared * randSquared * 2.f;
				const auto dia = rad * 2.f;
				const auto xF = static_cast<float>(x);
				const auto yF = static_cast<float>(y);
				const auto x1 = flipxF(xF - rad + rand.nextFloat() * dia);
				const auto y1 = flipyF(yF - rad + rand.nextFloat() * dia);
				g.fillEllipse(x1 - rad, y1 - rad, dia, dia);
			}
		}
		if (numSpotsFound == 0)
		{
			g.setColour(Colour::fromHSL(rand.nextFloat(), rand.nextFloat(), rand.nextFloat(), 1.f));
			const auto x = rand.nextInt(width);
			const auto y = rand.nextInt(height);
			const auto xF = static_cast<float>(x);
			const auto yF = static_cast<float>(y);
			const auto randSquared = rand.nextFloat();
			const auto rad = 1.f + randSquared * randSquared * 13.f;
			const auto dia = rad * 2.f;
			g.fillEllipse(xF - rad, yF - rad, dia, dia);
		}
		else if (numSpotsFound == numSteps)
		{
			g.setColour(Colour(0xff000000));
			const auto x = rand.nextInt(width);
			const auto y = rand.nextInt(height);
			const auto xF = static_cast<float>(x);
			const auto yF = static_cast<float>(y);
			const auto randSquared = rand.nextFloat();
			const auto rad = randSquared * static_cast<float>(height);
			const auto dia = rad * 2.f;
			g.fillEllipse(xF - rad, yF - rad, dia, dia);
		}

		auto brightness = 0.f;
		for (auto y = 0; y < height; ++y)
			for (auto x = 0; x < width; ++x)
			{
				const auto pxl = img.getPixelAt(x, y);
				brightness += pxl.getBrightness();
			}
		brightness /= width * height;
		const auto brightnessMargin = .1f;
		if (brightness < brightnessMargin || brightness > 1.f - brightnessMargin)
		{
			const auto minDimen = juce::jmin(width, height);
			const Point randPos(rand.nextInt(width), rand.nextInt(height));
			const auto randRad = rand.nextFloat() * minDimen;
			const auto randCol = Colour::fromHSL(rand.nextFloat(), rand.nextFloat(), rand.nextFloat(), 1.f);
			g.setColour(randCol);
			g.fillEllipse(randPos.x - randRad, randPos.y - randRad, randRad * 2.f, randRad * 2.f);
		}
	}

	// TREE PROCESS:
	void GenAniGrowTrees::startNewBranch(float width, float height, float gravity, float latchLikelyness) noexcept
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

	void GenAniGrowTrees::darken(float valTop, float valBtm) noexcept
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

	void GenAniGrowTrees::brighten(float valTop, float valBtm) noexcept
	{
		Colour white(0xffffffff);
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
				pxl = pxl.interpolatedWith(white, val);
				img.setPixelAt(x, y, pxl);
			}
		}
	}

	void GenAniGrowTrees::genNewCol(float mixOldNew, int tolerance) noexcept
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

	void GenAniGrowTrees::mutate(float width, float height)
	{
		for (auto i = 0; i < 64; ++i)
			makePoroes(width, height);
		darken(1.f, .4f);
		genNewCol(.7f, 3);
		startNewBranch(width, height, 12.f, .7f);
		alphaDownCount = 0;
	}

	void GenAniGrowTrees::makePoroes(float width, float height)
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
	// TREE PROCESS END
}