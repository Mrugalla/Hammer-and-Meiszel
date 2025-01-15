#pragma once
#include "Label.h"

namespace gui
{
    struct Ruler :
        public Comp
    {
		using Labels = std::vector<std::unique_ptr<Label>>;

		Ruler(Utils&);

		// https://oeis.org/search?q=1%2C2%2C5%2C10%2C20%2C50%2C100%2C200%2C500%2C1000&language=english&go=Search
		void makeIncExpansionOfGF();

		void setGetIncFunc(std::function<float(float)>&&) noexcept;

		void setValToStrFunc(std::function<String(float)>&&) noexcept;

		void setDrawFirstVal(bool) noexcept;

		void paint(Graphics&) override;

		void resized() override;

		void setLength(float) noexcept;

		void update();

		void setCID(CID) noexcept;

		const Labels& getLabels() const noexcept;

		// g, top, btm, numStripes
		void paintStripes(Graphics&, float, float, int);

		// val, inc
		double getSnappedToGrid(double, double) const noexcept;

		double getNextHigherSnapped(double) const noexcept;

		double getNextLowerSnapped(double) const noexcept;

	protected:
		Labels labels;
		std::function<float(float)> getIncFunc;
		std::function<String(float)> valToStrFunc;
		float length;
		CID cID;
		bool drawFirstVal;
    };
}