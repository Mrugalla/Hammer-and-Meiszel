#pragma once
#include "Comp.h"

namespace gui
{
	struct GenAniComp :
		public Comp
	{
		// u, tooltip
		GenAniComp(Utils&, String&&);

		~GenAniComp();

		void init();

		void paint(Graphics&) override;

		bool loadImage();

		void saveImage();

		void mouseUp(const Mouse&) override;

		void resized() override;
	protected:
		Image img;
		std::function<void()> onResize;
		int mode, numModes;
		bool active;
	};

	struct GenAniGrowTrees :
		public GenAniComp
	{
		enum { kTree, kTech, kNumModes };
		// TREE CONSTANTS
		static constexpr float MutationLikelyness = .05f;
		static constexpr int MutationTime = 9;
		static constexpr float NewBranchMinY = .6f;
		//

		GenAniGrowTrees(Utils&);
	private:
		Random rand;
		// TREE VARIABLES
		juce::Colour col;
		PointF pos;
		float angle;
		int alphaDownCount;
		// TECH VARIABLES
		//

		void treeProcess(Graphics&);

		void techProcess(Graphics&);

		// TREE PROCESS:
		//width, height, gravity, latchLikelyness
		void startNewBranch(float, float, float, float) noexcept;

		// valTop, valBtm
		void darken(float, float) noexcept;

		// valTop, valBtm
		void brighten(float, float) noexcept;

		// mixOldNew, tolerance
		void genNewCol(float, int) noexcept;

		// width, height
		void mutate(float, float);

		// width, height
		void makePoroes(float, float);
		// TREE PROCESS END
	};
}
