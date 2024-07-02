#pragma once
#include "Using.h"

namespace gui
{
	void hideCursor();

	void showCursor(const Component&);

	void showCursor(const PointF);

	void centreCursor(const Component&, juce::MouseInputSource&);

	/* text, randomizer, length, legalChars*/
	void appendRandomString(String&, Random&, int,
		const String& = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

	BoundsF smallestBoundsIn(const LineF&) noexcept;

	BoundsF maxQuadIn(const BoundsF&) noexcept;

	BoundsF maxQuadIn(const Bounds&) noexcept;

	void repaintWithChildren(Component*);

	std::unique_ptr<juce::XmlElement> loadXML(const char*, const int);

	struct Layout
	{
		Layout();

		// xDist, yDist
		void init(const std::vector<int>&, const std::vector<int>&);

		// xDist, yDist
		void initFromStrings(const String&, const String&);

		// numStepsX, numStepsY
		void initGrid(int, int);

		void resized(Bounds) noexcept;

		void resized(BoundsF) noexcept;

		template<typename X, typename Y>
		PointF operator()(X, Y) const noexcept;

		template<typename PointType>
		PointF operator()(PointType) const noexcept;

		template<typename X, typename Y>
		BoundsF operator()(X, Y, X, Y, bool /*isQuad*/ = false) const noexcept;

		template<typename PointType0, typename PointType1>
		LineF getLine(PointType0, PointType1) const noexcept;

		template<typename X0, typename Y0, typename X1, typename Y1>
		LineF getLine(X0, Y0, X1, Y1) const noexcept;

		BoundsF bottom(bool /*isQuad*/ = false) const noexcept;

		BoundsF top(bool /*isQuad*/ = false) const noexcept;

		BoundsF right(bool /*isQuad*/ = false) const noexcept;

		BoundsF cornerTopRight() const noexcept;

		float getX(int) const noexcept;
		float getY(int) const noexcept;
		float getX(float) const noexcept;

		float getY(float) const noexcept;

		template<typename X>
		float getW(X) const noexcept;

		template<typename Y>
		float getH(Y) const noexcept;

		template<typename X, typename Y>
		void place(Component&, X, Y y, X = static_cast<X>(1), Y = static_cast<Y>(1), bool /*isQuad*/ = false) const noexcept;

		template<typename X, typename Y>
		void place(Component*, X, Y, X = static_cast<X>(1), Y = static_cast<Y>(1), bool /*isQuad*/ = false) const noexcept;

		void paint(Graphics&);

		void paint(Graphics&, Colour);

		template<typename X, typename Y>
		void label(Graphics&, String&&, X, Y, X = static_cast<X>(1), Y = static_cast<Y>(1), bool /*isQuad*/ = false) const;

	protected:
		std::vector<float> rXRaw, rX;
		std::vector<float> rYRaw, rY;
	};

	void make(Path&, const Layout&, std::vector<Point>&&);

	/* g, y, left, right, thicc */
	void drawHorizontalLine(Graphics&, int, float, float, int = 1);

	/* g, x, top, bottom, thicc */
	void drawVerticalLine(Graphics&, int, float, float, int = 1);

	/* graphics, bounds, edgeWidth, edgeHeight, stroke */
	void drawRectEdges(Graphics&, const BoundsF&, float, float, const Stroke&);

	/* graphics, bounds, edgeWidth, stroke */
	void drawRectEdges(Graphics&, const BoundsF&, float, const Stroke&);

	/* graphics, bounds, text */
	void drawHeadLine(Graphics&, const BoundsF&, const String&);

	inline void visualizeGroupNEL(Graphics& g, BoundsF bounds, float thicc)
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

	// graphics, text, bounds, colour, thicc
	void visualizeGroupNEL(Graphics&, String&&,
		BoundsF, Colour, float);

	template<class ArrayCompPtr>
	inline void distributeVertically(const Component& parent, ArrayCompPtr& compArray)
	{
		const auto w = static_cast<float>(parent.getWidth());
		const auto h = static_cast<float>(parent.getHeight());
		const auto x = 0.f;

		auto y = 0.f;

		const auto cH = h / static_cast<float>(compArray.size());

		for (auto& cmp : compArray)
		{
			cmp->setBounds(BoundsF(x, y, w, cH).toNearestInt());
			y += cH;
		}
	}

	template<class SizeType>
	inline void distributeVertically(const Component& parent, Component* compArray, SizeType size)
	{
		const auto w = static_cast<float>(parent.getWidth());
		const auto h = static_cast<float>(parent.getHeight());
		const auto x = 0.f;

		auto y = 0.f;

		const auto cH = h / static_cast<float>(size);

		for (auto i = 0; i < size; ++i)
		{
			auto& cmp = compArray[i];
			cmp.setBounds(BoundsF(x, y, w, cH).toNearestInt());
			y += cH;
		}
	}

	PointF boundsOf(const Font&, const String&) noexcept;

	float findMaxHeight(const Font&, const String&, float, float) noexcept;

	// lrud = left, right, up, down [0, 1, 2, 3]
	inline void closePathOverBounds(Path& p, const BoundsF& bounds, const PointF& endPos, float thicc, int lrud0, int lrud1, int lrud2, int lrud3)
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

	namespace imgPP
	{
		void blur(Image&, Graphics&, int /*its*/ = 3) noexcept;
	}
}