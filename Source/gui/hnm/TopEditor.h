#pragma once
#include "../ButtonRandomizer.h"
#include "../PatchBrowser.h"

namespace gui
{
	struct TopEditor :
		public Comp
	{
		TopEditor(Utils&, patch::Browser&);

		void resized() override;

		void paint(Graphics&) override;

	protected:
		patch::BrowserButton patchBrowserButton;
		patch::NextPatchButton previous, next;
		ButtonRandomizer buttonRandomizer;
		Button buttonSoftClip;
	};
}