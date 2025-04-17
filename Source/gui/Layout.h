#pragma once
#include "Using.h"

namespace gui
{
	Mouse generateFakeMouseEvent(const juce::MouseInputSource&) noexcept;

	void fixStupidJUCEImageThingie(Image&);

	void hideCursor();

	void showCursor(const Component&);

	void showCursor(const PointF);

	void centreCursor(const Component&, juce::MouseInputSource&);

	// text, randomizer, length, legalChars
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

	// g, y, left, right, thicc
	void drawHorizontalLine(Graphics&, int, float, float, int = 1);

	// g, x, top, bottom, thicc
	void drawVerticalLine(Graphics&, int, float, float, int = 1);

	// graphics, bounds, edgeWidth, edgeHeight, stroke
	void drawRectEdges(Graphics&, const BoundsF&, float, float, const Stroke&);

	// graphics, bounds, edgeWidth, stroke
	void drawRectEdges(Graphics&, const BoundsF&, float, const Stroke&);

	// graphics, bounds, text
	void drawHeadLine(Graphics&, const BoundsF&, const String&);

	// g, bounds, thicc
	void visualizeGroupNEL(Graphics&, BoundsF, float);

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

	// font, text, width, height
	float findMaxHeight(const Font&, const String&, float, float) noexcept;

	// path, bounds, endPos, thicc, 0, 1, 2, 3
	// lrud = left, right, up, down [0, 1, 2, 3]
	void closePathOverBounds(Path&, const BoundsF&, const PointF&,
		float, int, int, int, int);

	namespace imgPP
	{
		// img, g, iterations
		void blur(Image&, Graphics&, int = 3) noexcept;
	}
}