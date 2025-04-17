#include "Layout.h"

namespace gui
{
	Mouse generateFakeMouseEvent(const juce::MouseInputSource& src) noexcept
	{
		const auto now = juce::Time::getCurrentTime();
		const PointF pos;
		Mouse mouse(src, pos, juce::ModifierKeys(), 0.f, 0.f, 0.f, 0.f, 0.f, nullptr, nullptr, now, pos, now, 1, false);
		return mouse;
	}

	void fixStupidJUCEImageThingie(Image& img)
	{
		img = juce::SoftwareImageType().convert(img);
	}

	void hideCursor()
	{
		auto mms = juce::Desktop::getInstance().getMainMouseSource();
		mms.enableUnboundedMouseMovement(true, false);
	}

	void showCursor(const Component& comp)
	{
		auto mms = juce::Desktop::getInstance().getMainMouseSource();
		centreCursor(comp, mms);
		mms.enableUnboundedMouseMovement(false, true);
	}

	void showCursor(const PointF pt)
	{
		auto mms = juce::Desktop::getInstance().getMainMouseSource();
		mms.setScreenPosition(pt);
		mms.enableUnboundedMouseMovement(false, true);
	}

	void centreCursor(const Component& comp, juce::MouseInputSource& mms)
	{
		const Point centre(comp.getWidth() / 2, comp.getHeight() / 2);
		mms.setScreenPosition((comp.getScreenPosition() + centre).toFloat());
	}

	void appendRandomString(String& str, Random& rand, int length, const String& legalChars)
	{
		const auto max = static_cast<float>(legalChars.length() - 1);

		for (auto i = 0; i < length; ++i)
		{
			auto idx = static_cast<int>(rand.nextFloat() * max);
			str += legalChars[idx];
		}
	}

	BoundsF smallestBoundsIn(const LineF& line) noexcept
	{
		return { line.getStart(), line.getEnd() };
	}

	BoundsF maxQuadIn(const BoundsF& b) noexcept
	{
		const auto minDimen = std::min(b.getWidth(), b.getHeight());
		const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
		const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
		return { x, y, minDimen, minDimen };
	}

	BoundsF maxQuadIn(const Bounds& b) noexcept
	{
		return maxQuadIn(b.toFloat());
	}

	void repaintWithChildren(Component* comp)
	{
		if (comp == nullptr)
			return;
		comp->repaint();
		for (auto c = 0; c < comp->getNumChildComponents(); ++c)
			repaintWithChildren(comp->getChildComponent(c));
	}

	std::unique_ptr<juce::XmlElement> loadXML(const char* data, const int sizeInBytes)
	{
		return juce::XmlDocument::parse(String(data, sizeInBytes));
	}

	Layout::Layout() :
		rXRaw(),
		rYRaw(),
		rX(),
		rY()
	{
	}

	void Layout::init(const std::vector<int>& xDist, const std::vector<int>& yDist)
	{
		const auto numCols = xDist.size();
		const auto numRows = yDist.size();
		rXRaw.reserve(numCols + 2);
		rYRaw.reserve(numRows + 2);

		rXRaw.emplace_back(0.f);
		rYRaw.emplace_back(0.f);

		{ // normalize input values Cols
			auto sum = 0.f;
			for (auto i = 0; i < numCols; ++i)
			{
				rXRaw.emplace_back(static_cast<float>(xDist[i]));
				sum += static_cast<float>(xDist[i]);
			}
			const auto g = 1.f / sum;
			for (auto& i : rXRaw)
				i *= g;
		}
		{ // normalize input values Rows
			auto sum = 0.f;
			for (auto i = 0; i < numRows; ++i)
			{
				rYRaw.emplace_back(static_cast<float>(yDist[i]));
				sum += static_cast<float>(yDist[i]);
			}
			const auto g = 1.f / sum;
			for (auto& i : rYRaw)
				i *= g;
		}

		rXRaw.emplace_back(0.f);
		rYRaw.emplace_back(0.f);

		for (auto x = 1; x < rXRaw.size(); ++x)
			rXRaw[x] += rXRaw[x - 1];
		for (auto y = 1; y < rYRaw.size(); ++y)
			rYRaw[y] += rYRaw[y - 1];

		rX = rXRaw;
		rY = rYRaw;
	}

	void Layout::initFromStrings(const String& xStr, const String& yStr)
	{
		std::vector<int> xDist, yDist;

		int sIdx = 0;
		for (auto i = 0; i < xStr.length(); ++i)
		{
			if (xStr[i] == ';')
			{
				xDist.push_back(xStr.substring(sIdx, i).getIntValue());
				++i;
				sIdx = i;
			}
		}
		xDist.push_back(xStr.substring(sIdx).getIntValue());
		sIdx = 0;
		for (auto i = 0; i < yStr.length(); ++i)
		{
			if (yStr[i] == ';')
			{
				yDist.push_back(yStr.substring(sIdx, i).getIntValue());
				++i;
				sIdx = i;
			}
		}
		yDist.push_back(yStr.substring(sIdx).getIntValue());

		init(xDist, yDist);
	}

	void Layout::initGrid(int numX, int numY)
	{
		init
		(
			std::vector<int>(numX, 1),
			std::vector<int>(numY, 1)
		);
	}

	void Layout::resized(Bounds bounds) noexcept
	{
		resized(bounds.toFloat());
	}

	void Layout::resized(BoundsF bounds) noexcept
	{
		for (auto x = 0; x < rX.size(); ++x)
			rX[x] = rXRaw[x] * bounds.getWidth();
		for (auto y = 0; y < rY.size(); ++y)
			rY[y] = rYRaw[y] * bounds.getHeight();
		for (auto& x : rX)
			x += bounds.getX();
		for (auto& y : rY)
			y += bounds.getY();
	}

	template<typename X, typename Y>
	PointF Layout::operator()(X x, Y y) const noexcept
	{
		return { getX(x), getY(y) };
	}

	template<typename PointType>
	PointF Layout::operator()(PointType pt) const noexcept
	{
		return { getX(pt.x), getY(pt.y) };
	}

	template<typename X, typename Y>
	BoundsF Layout::operator()(X x, Y y, X width, Y height, bool isQuad) const noexcept
	{
		const auto x0 = getX(x);
		const auto y0 = getY(y);
		const auto w0 = getX(x + width) - x0;
		const auto h0 = getY(y + height) - y0;

		BoundsF nBounds(x0, y0, w0, h0);
		return isQuad ? maxQuadIn(nBounds) : nBounds;
	}

	template<typename PointType0, typename PointType1>
	LineF Layout::getLine(PointType0 p0, PointType1 p1) const noexcept
	{
		return { getX(p0.x), getY(p0.y), getX(p1.x), getY(p1.y) };
	}

	template<typename X0, typename Y0, typename X1, typename Y1>
	LineF Layout::getLine(X0 x0, Y0 y0, X1 x1, Y1 y1) const noexcept
	{
		return { getX(x0), getY(y0), getX(x1), getY(y1) };
	}

	BoundsF Layout::bottom(bool isQuad) const noexcept
	{
		const auto w = rX.back();
		const auto y = getY(static_cast<int>(rY.size() - 3));
		const auto h = getY(static_cast<int>(rY.size() - 2)) - y;
		BoundsF nBounds(0.f, y, w, h);
		return isQuad ? maxQuadIn(nBounds) : nBounds;
	}

	BoundsF Layout::top(bool isQuad) const noexcept
	{
		const auto w = rX.back();
		const auto h = getY(1);
		BoundsF nBounds(0.f, 0.f, w, h);
		return isQuad ? maxQuadIn(nBounds) : nBounds;
	}

	BoundsF Layout::right(bool isQuad) const noexcept
	{
		const auto y = 0.f;
		const auto x = getX(static_cast<int>(rX.size() - 3));
		const auto w = getX(static_cast<int>(rX.size() - 2)) - x;
		const auto h = rY.back();

		BoundsF nBounds(x, y, w, h);
		return isQuad ? maxQuadIn(nBounds) : nBounds;
	}

	BoundsF Layout::cornerTopRight() const noexcept
	{
		const auto x = getX(static_cast<int>(rX.size() - 3));
		const auto y = 0.f;
		const auto w = rX.back() - x;
		const auto h = getY(1);

		return { x, y, w, h };
	}

	float Layout::getX(int i) const noexcept
	{
		if(i >= 0)
			return rX[i];
		else
			return rX[rX.size() + i - 1];
	}

	float Layout::getY(int i) const noexcept
	{
		if (i >= 0)
			return rY[i];
		else
			return rY[rY.size() + i - 1];
	}

	float Layout::getX(float i) const noexcept
	{
		const auto f = std::floor(i);
		const auto iF = static_cast<int>(f);
		const auto iC = iF + 1;
		const auto x = i - f;

		const auto xF = getX(iF);
		const auto xC = getX(iC);

		return xF + x * (xC - xF);
	}

	float Layout::getY(float i) const noexcept
	{
		const auto f = std::floor(i);
		const auto iF = static_cast<int>(f);
		const auto iC = iF + 1;
		const auto y = i - f;

		const auto yF = getY(iF);
		const auto yC = getY(iC);

		return yF + y * (yC - yF);
	}

	template<typename X>
	float Layout::getW(X i) const noexcept
	{
		return static_cast<float>(getX(i + static_cast<X>(1)) - getX(i));
	}

	template<typename Y>
	float Layout::getH(Y i) const noexcept
	{
		return static_cast<float>(getY(i + static_cast<Y>(1)) - getY(i));
	}

	template<typename X, typename Y>
	void Layout::place(Component& childComp, X x, Y y, X width, Y height, bool isQuad) const noexcept
	{
		const auto cBounds = operator()(x, y, width, height, isQuad);
		childComp.setBounds(cBounds.toNearestInt());
	}

	template<typename X, typename Y>
	void Layout::place(Component* childComp, X x, Y y, X width, Y height, bool isQuad) const noexcept
	{
		if (childComp == nullptr) return;
		place(*childComp, x, y, width, height, isQuad);
	}

	void Layout::paint(Graphics& g)
	{
		const auto btm = bottom();
		
		for (auto x = 1; x < rX.size() - 1; ++x)
			g.drawVerticalLine(static_cast<int>(rX[x]), rY[0], btm.getBottom());
		for (auto y = 1; y < rY.size() - 1; ++y)
			g.drawHorizontalLine(static_cast<int>(rY[y]), rX[0], btm.getRight());
	}

	void Layout::paint(Graphics& g, Colour c)
	{
		g.setColour(c);
		paint(g);
	}

	template<typename X, typename Y>
	void Layout::label(Graphics& g, String&& txt, X x, Y y, X width, Y height, bool isQuad) const
	{
		const auto bounds = operator()(x, y, width, height, isQuad);
		g.drawRect(bounds);
		g.drawFittedText(txt, bounds.toNearestInt(), Just::centred, 1);
	}

	void make(Path& path, const Layout& layout, std::vector<Point>&& points)
	{
		path.startNewSubPath(layout(points[0]));
		for (auto i = 1; i < points.size(); ++i)
			path.lineTo(layout(points[i]));
	}

	void drawHorizontalLine(Graphics& g, int y, float left, float right, int thicc)
	{
		g.drawHorizontalLine(y, left, right);
		for (auto t = 1; t < thicc; ++t)
		{
			g.drawHorizontalLine(y + t, left, right);
			g.drawHorizontalLine(y - t, left, right);
		}
	}

	void drawVerticalLine(Graphics& g, int x, float top, float bottom, int thicc)
	{
		g.drawVerticalLine(x, top, bottom);
		for (auto t = 1; t < thicc; ++t)
		{
			g.drawVerticalLine(x + t, top, bottom);
			g.drawVerticalLine(x - t, top, bottom);
		}
	}

	void drawRectEdges(Graphics& g, const BoundsF& bounds,
		float edgeWidth, float edgeHeight, const Stroke& st)
	{
		const auto x = bounds.getX();
		const auto y = bounds.getY();
		const auto right = bounds.getRight();
		const auto bottom = bounds.getBottom();

		const auto xPlusEdge = x + edgeWidth;
		const auto yPlusEdge = y + edgeHeight;
		const auto rightMinusEdge = right - edgeWidth;
		const auto bottomMinusEdge = bottom - edgeHeight;

		Path path;
		path.startNewSubPath(x, yPlusEdge);
		path.lineTo(x, y);
		path.lineTo(xPlusEdge, y);

		path.startNewSubPath(x, bottomMinusEdge);
		path.lineTo(x, bottom);
		path.lineTo(xPlusEdge, bottom);

		path.startNewSubPath(rightMinusEdge, bottom);
		path.lineTo(right, bottom);
		path.lineTo(right, bottomMinusEdge);

		path.startNewSubPath(right, yPlusEdge);
		path.lineTo(right, y);
		path.lineTo(rightMinusEdge, y);

		g.strokePath(path, st);
	}

	void drawRectEdges(Graphics& g, const BoundsF& bounds,
		float edgeWidth, const Stroke& st)
	{
		drawRectEdges(g, bounds, edgeWidth, edgeWidth, st);
	}

	void drawHeadLine(Graphics& g, const BoundsF& bounds, const String& txt)
	{
		g.drawFittedText(txt, bounds.toNearestInt(), Just::centredTop, 1);
	}

	void visualizeGroupNEL(Graphics& g, BoundsF bounds, float thicc)
	{
		Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);

		{
			const auto x = bounds.getX();
			const auto y = bounds.getY();
			const auto w = bounds.getWidth();
			const auto h = bounds.getHeight();

			const auto midDimen = std::min(w, h);

			const auto x0 = x;
			const auto x125 = x + .125f * midDimen;
			const auto x25 = x + .25f * midDimen;
			const auto x75 = x + w - .25f * midDimen;
			const auto x875 = x + w - .125f * midDimen;
			const auto x1 = x + w;

			const auto y0 = y;
			const auto y125 = y + .125f * midDimen;
			const auto y25 = y + .25f * midDimen;
			const auto y75 = y + h - .25f * midDimen;
			const auto y875 = y + h - .125f * midDimen;
			const auto y1 = y + h;

			Path p;
			p.startNewSubPath(x0, y25);
			p.lineTo(x0, y125);
			p.lineTo(x125, y0);
			p.lineTo(x25, y0);
			for (auto i = 1; i < 3; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto n = iF / 3.f;

				const auto nY = y0 + n * (y125 - y0);
				const auto nX = x0 + n * (x125 - x0);

				p.startNewSubPath(x0, nY);
				p.lineTo(nX, y0);
			}

			p.startNewSubPath(x875, y0);
			p.lineTo(x1, y0);
			p.lineTo(x1, y125);

			p.startNewSubPath(x1, y75);
			p.lineTo(x1, y875);
			p.lineTo(x875, y1);
			p.lineTo(x75, y1);
			for (auto i = 1; i < 3; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto n = iF / 3.f;

				const auto nY = y1 + n * (y875 - y1);
				const auto nX = x1 + n * (x875 - x1);

				p.startNewSubPath(x1, nY);
				p.lineTo(nX, y1);
			}

			for (auto i = 1; i <= 3; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto n = iF / 3.f;

				const auto nY = y1 + n * (y875 - y1);
				const auto nX = x0 + n * (x125 - x0);

				p.startNewSubPath(x0, nY);
				p.lineTo(nX, y1);
			}

			g.strokePath(p, stroke);
		}
	}

	void visualizeGroupNEL(Graphics& g, String&& txt,
		BoundsF bounds, Colour col, float thicc)
	{
		Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
		g.setColour(col);
		{
			const auto x = bounds.getX();
			const auto y = bounds.getY();
			const auto w = bounds.getWidth();
			const auto h = bounds.getHeight();

			const auto midDimen = std::min(w, h);

			const auto x0 = x;
			const auto x125 = x + .125f * midDimen;
			const auto x25 = x + .25f * midDimen;
			const auto x75 = x + w - .25f * midDimen;
			const auto x875 = x + w - .125f * midDimen;
			const auto x1 = x + w;

			const auto y0 = y;
			const auto y125 = y + .125f * midDimen;
			const auto y25 = y + .25f * midDimen;
			const auto y75 = y + h - .25f * midDimen;
			const auto y875 = y + h - .125f * midDimen;
			const auto y1 = y + h;

			Path p;
			p.startNewSubPath(x0, y25);
			p.lineTo(x0, y125);
			p.lineTo(x125, y0);
			p.lineTo(x25, y0);
			for (auto i = 1; i < 3; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto n = iF / 3.f;

				const auto nY = y0 + n * (y125 - y0);
				const auto nX = x0 + n * (x125 - x0);

				p.startNewSubPath(x0, nY);
				p.lineTo(nX, y0);
			}

			p.startNewSubPath(x875, y0);
			p.lineTo(x1, y0);
			p.lineTo(x1, y125);

			p.startNewSubPath(x1, y75);
			p.lineTo(x1, y875);
			p.lineTo(x875, y1);
			p.lineTo(x75, y1);
			for (auto i = 1; i < 3; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto n = iF / 3.f;

				const auto nY = y1 + n * (y875 - y1);
				const auto nX = x1 + n * (x875 - x1);

				p.startNewSubPath(x1, nY);
				p.lineTo(nX, y1);
			}

			for (auto i = 1; i <= 3; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto n = iF / 3.f;

				const auto nY = y1 + n * (y875 - y1);
				const auto nX = x0 + n * (x125 - x0);

				p.startNewSubPath(x0, nY);
				p.lineTo(nX, y1);
			}

			g.strokePath(p, stroke);

			if (txt.isNotEmpty())
			{
				auto tFont = font::nel();
				BoundsF area
				(
					x75, y0,
					x1 - x75,
					y25 - y0
				);
				auto maxHeight = findMaxHeight(tFont, txt, area.getWidth(), area.getHeight());
				tFont.setHeight(maxHeight);
				g.setFont(tFont);
				g.drawFittedText(txt, area.toNearestInt(), juce::Justification::centred, 1);
			}
		}
	}

	PointF boundsOf(const Font& font, const String& text) noexcept
	{
		auto maxStrWidth = 0.f;
		auto numLines = 1.f;
		{
			auto sIdx = 0;
			for (auto i = 1; i < text.length(); ++i)
			{
				const auto chr = text[i];
				if (isLineBreak(chr))
				{
					const auto lineWidth = GlyphArrangement::getStringWidth(font, text.substring(sIdx, i));
					if (maxStrWidth < lineWidth)
						maxStrWidth = lineWidth;
					sIdx = i + 1;
					++numLines;
				}
			}
			const auto lineWidth = GlyphArrangement::getStringWidth(font, text.substring(sIdx));
			if (maxStrWidth < lineWidth)
				maxStrWidth = lineWidth;
		}

		const auto fontHeight = font.getHeight();
		const auto strHeight = fontHeight * numLines;

		return { maxStrWidth, strHeight };
	}

	float findMaxHeight(const Font& font, const String& text,
		float width, float height) noexcept
	{
		const auto oHeight = font.getHeight();
		if (text.isEmpty())
			return oHeight;
		const auto fontBounds = boundsOf(font, text);
		const auto ratio = std::min(width / fontBounds.x, height / fontBounds.y);
		const auto nHeight = oHeight * ratio;
		return nHeight;
	}

	void closePathOverBounds(Path& p, const BoundsF& bounds, const PointF& endPos,
		float thicc, int lrud0, int lrud1, int lrud2, int lrud3)
	{
		const auto startPos = p.getCurrentPosition();
		auto x = lrud0 == 0 ? -thicc : lrud0 == 1 ? bounds.getWidth() + thicc : startPos.x;
		auto y = lrud0 <= 1 ? startPos.y : lrud0 == 2 ? -thicc : bounds.getHeight() + thicc;
		p.lineTo(x, y);
		x = lrud1 == 0 ? -thicc : lrud1 == 1 ? bounds.getWidth() + thicc : x;
		y = lrud1 <= 1 ? y : lrud1 == 2 ? -thicc : bounds.getHeight() + thicc;
		p.lineTo(x, y);
		x = lrud2 == 0 ? -thicc : lrud2 == 1 ? bounds.getWidth() + thicc : x;
		y = lrud2 <= 1 ? y : lrud2 == 2 ? -thicc : bounds.getHeight() + thicc;
		p.lineTo(x, y);
		x = lrud3 == 0 ? -thicc : lrud3 == 1 ? bounds.getWidth() + thicc : x;
		y = lrud3 <= 1 ? y : lrud3 == 2 ? -thicc : bounds.getHeight() + thicc;
		p.lineTo(x, y);
		if (endPos.x < 1.f || endPos.x >= bounds.getWidth() - 1.f)
			y = endPos.y;
		else if (endPos.y < 1.f || endPos.y >= bounds.getHeight() - 1.f)
			x = endPos.x;
		p.lineTo(x, y);
		p.closeSubPath();
	}

	void imgPP::blur(Image& img, Graphics& g, int its) noexcept
	{
		const auto w = img.getWidth();
		const auto h = img.getHeight();

		std::array<Point, 4> pt
		({
			Point{-1, -1}, Point{-1, -0},
			Point{-0, -1}, Point{-0, -0}
		});

		std::array<float, 3> rgb = { 0.f, 0.f, 0.f };
		enum { R, G, B };

		Image::BitmapData bitmap(img, Image::BitmapData::ReadWriteMode::readWrite);

		for (auto j = 0; j < its; ++j)
			for (auto y = 1; y < h; ++y)
				for (auto x = 1; x < w; ++x)
				{
					rgb[R] = rgb[G] = rgb[B] = 0.f;
					for (auto i = 0; i < pt.size(); ++i)
					{
						const auto pxl = bitmap.getPixelColour(x + pt[i].x, y + pt[i].y);
						rgb[R] += pxl.getFloatRed();
						rgb[G] += pxl.getFloatGreen();
						rgb[B] += pxl.getFloatBlue();
					}
					for (auto i = 0; i < rgb.size(); ++i)
						rgb[i] *= .25f;

					const auto nPxl = Colour::fromFloatRGBA(rgb[R], rgb[G], rgb[B], 1.f);

					bitmap.setPixelColour(x, y, nPxl);
				}

		g.drawImageAt(img, 0, 0, false);
	}

	template void Layout::label<int, int>(Graphics&, String&&, int, int, int, int, bool) const;
	template void Layout::label<float, int>(Graphics&, String&&, float, int, float, int, bool) const;
	template void Layout::label<int, float>(Graphics&, String&&, int, float, int, float, bool) const;
	template void Layout::label<float, float>(Graphics&, String&&, float, float, float, float, bool) const;

	template PointF Layout::operator()<int, int>(int, int) const noexcept;
	template PointF Layout::operator()<float, int>(float, int) const noexcept;
	template PointF Layout::operator()<int, float>(int, float) const noexcept;
	template PointF Layout::operator()<float, float>(float, float) const noexcept;

	template PointF Layout::operator() < gui::Point > (Point) const noexcept;
	template PointF Layout::operator() < gui::PointF > (PointF) const noexcept;

	template LineF Layout::getLine<Point, Point>(Point, Point) const noexcept;
	template LineF Layout::getLine<PointF, Point>(PointF, Point) const noexcept;
	template LineF Layout::getLine<Point, PointF>(Point, PointF) const noexcept;
	template LineF Layout::getLine<PointF, PointF>(PointF, PointF) const noexcept;

	template float Layout::getW<int>(int) const noexcept;
	template float Layout::getW<float>(float) const noexcept;
	template float Layout::getH<int>(int) const noexcept;
	template float Layout::getH<float>(float) const noexcept;

	template LineF Layout::getLine<int, int>(int, int, int, int) const noexcept;
	template LineF Layout::getLine<int, float>(int, float, int, float) const noexcept;
	template LineF Layout::getLine<float, int>(float, int, float, int) const noexcept;
	template LineF Layout::getLine<float, float>(float, float, float, float) const noexcept;

	template BoundsF Layout::operator() < int, int > (int, int, int, int, bool) const noexcept;
	template BoundsF Layout::operator() < float, int > (float, int, float, int, bool) const noexcept;
	template BoundsF Layout::operator() < int, float > (int, float, int, float, bool) const noexcept;
	template BoundsF Layout::operator() < float, float > (float, float, float, float, bool) const noexcept;

	template void Layout::place<int, int>(Component&, int, int, int, int, bool) const noexcept;
	template void Layout::place<float, int>(Component&, float, int, float, int, bool) const noexcept;
	template void Layout::place<int, float>(Component&, int, float, int, float, bool) const noexcept;
	template void Layout::place<float, float>(Component&, float, float, float, float, bool) const noexcept;
	template void Layout::place<int, int>(Component*, int, int, int, int, bool) const noexcept;
	template void Layout::place<float, int>(Component*, float, int, float, int, bool) const noexcept;
	template void Layout::place<int, float>(Component*, int, float, int, float, bool) const noexcept;
	template void Layout::place<float, float>(Component*, float, float, float, float, bool) const noexcept;
}