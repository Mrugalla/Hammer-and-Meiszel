#pragma once
#include "Button.h"

namespace gui
{
	struct BgImage :
		public Comp
	{
		enum { kUpdateBoundsCB, kNumCallbacks };

        BgImage(Utils&);

        void paint(Graphics&);

        void resized() override;

		/* forcedLoad */
		void updateBgImage(bool);

		Image img;
		Button refreshButton;
		std::function<void(Image&)> createImageFunc;
	};
}