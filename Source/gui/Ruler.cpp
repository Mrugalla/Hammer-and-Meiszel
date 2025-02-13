#include "Ruler.h"

namespace gui
{
	Ruler::Ruler(Utils& u) :
		Comp(u),
		labels(),
		getIncFunc([](float l) { return l / 4.f; }),
		valToStrFunc([](float v) { return String(v); }),
		length(0.f),
		cID(CID::Txt),
		drawFirstVal(true)
	{
	}

	void Ruler::makeIncExpansionOfGF()
	{
		getIncFunc = [](float length)
		{
			float v[] = { 1.f, 2.f, 5.f, .2f, .5f, 1.f };

			while (true)
			{
				if (length < v[0])
					return v[3];
				if (length < v[1])
					return v[4];
				if (length < v[2])
					return v[5];
				for (auto i = 0; i < 6; ++i)
					v[i] *= 10.f;
			}
		};
	}

	void Ruler::setGetIncFunc(std::function<float(float)>&& f) noexcept
	{
		getIncFunc = f;
	}

	void Ruler::setValToStrFunc(std::function<String(float)>&& f) noexcept
	{
		valToStrFunc = f;
	}

	void Ruler::setDrawFirstVal(bool e) noexcept
	{
		drawFirstVal = e;
	}

	void Ruler::paint(Graphics& g)
	{
		if (length == 0.f)
			return;
		const auto thiccInt = static_cast<int>(utils.thicc * 2.f);
		const auto h = static_cast<float>(getHeight());
		setCol(g, cID);
		for (auto i = 0; i < labels.size(); ++i)
		{
			const auto x = labels[i]->getX() - thiccInt;
			g.drawVerticalLine(x, 0.f, h);
		}
	}

	void Ruler::resized()
	{
		update();
	}

	void Ruler::setLength(float l) noexcept
	{
		if (length == l)
			return;
		length = l;
		update();
	}

	void Ruler::update()
	{
		for (auto& label : labels)
			removeChildComponent(label.get());
		labels.clear();

		if (length <= 0.f)
			return;

		const auto just = Just::centredLeft;
		const auto font = font::dosisRegular();

		const auto xOff = static_cast<int>(utils.thicc * 2.f);
		const auto w = getWidth();
		const auto wF = static_cast<float>(w);
		const auto h = getHeight();

		const auto inc = getIncFunc(length);
		const auto numLabels = static_cast<int>(std::ceil(length / inc));
		const auto lenInv = 1.f / length;
		labels.reserve(numLabels);
		auto val = drawFirstVal ? 0.f : inc;
		auto xF = drawFirstVal ? 0.f : val * lenInv * wF;
		for (auto i = 0; i < numLabels; ++i)
		{
			const auto x = static_cast<int>(xF);
			labels.emplace_back(new Label(utils));
			auto& label = *labels.back();
			makeTextLabel(label, valToStrFunc(val), font, just, cID, "");
			label.setBounds(x + xOff, 0, 100, h);
			label.setMaxHeight();
			val += inc;
			xF = val * lenInv * wF;
		}

		for (auto i = 0; i < numLabels; ++i)
			addAndMakeVisible(labels[i].get());
	}

	void Ruler::setCID(CID c) noexcept
	{
		cID = c;
	}

	const Ruler::Labels& Ruler::getLabels() const noexcept
	{
		return labels;
	}

	void Ruler::paintStripes(Graphics& g, float top, float btm, int numStripes)
	{
		const auto h = btm - top;
		const auto thiccInt = static_cast<int>(utils.thicc * 2.f);
		const auto numStripesInv = 1.f / static_cast<float>(numStripes);
		setCol(g, cID);
		for (auto i = 0; i < labels.size(); ++i)
		{
			auto& label = *labels[i];
			const auto x = label.getX() - thiccInt;

			for (auto j = 1; j < numStripes; ++j)
			{
				if (j % 2 == 0)
				{
					const auto jF = static_cast<float>(j);
					const auto j0 = jF;
					const auto j1 = j0 + 1.f;
					const auto j0Rat = j0 * numStripesInv;
					const auto j1Rat = j1 * numStripesInv;
					const auto j0Y = j0Rat * h;
					const auto j1Y = j1Rat * h;
					const auto j0Shift = j0Y + top;
					const auto j1Shift = j1Y + top;
					g.drawVerticalLine(x, j0Shift, j1Shift);
				}
			}
		}
	}

	double Ruler::getSnappedToGrid(double val, double inc) const noexcept
	{
		const auto a = std::floor((val - 1.) / inc);
		return a * inc + 1.;
	}

	double Ruler::getNextHigherSnapped(double val) const noexcept
	{
		const auto inc = getIncFunc(length);
		val += inc;
		return getSnappedToGrid(val, inc);
	}

	double Ruler::getNextLowerSnapped(double val) const noexcept
	{
		const auto inc = getIncFunc(length);
		val -= inc;
		return getSnappedToGrid(val, inc);
	}
}