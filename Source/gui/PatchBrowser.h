#pragma once
#include "TextEditor.h"
#include "../arch/State.h"

namespace gui
{
	namespace patch
	{
		static constexpr int NumPatches = 12;

		struct Patch :
			public Comp
		{
			Patch(Utils&);

			// name, author, file
			void activate(const String&, const String&, const File&);

			void deactivate();

			void resized() override;

			String name, author;
			File file;
			Button buttonLoad, buttonDelete;
		};

		struct ScrollBar :
			public Comp
		{
			ScrollBar(Utils&);

			void paint(Graphics&) override;

			void mouseWheelMove(const Mouse&, const MouseWheel&) override;

			std::function<void()> onScroll;
			int viewIdx, numFiles;
		};

		struct Patches :
			public Comp
		{
			Patches(Utils&);

			void resized() override;

			// name, author
			void updateFilter(const String&, const String&);

			void update();

			// file, i
			void updateAdd(const File&, int);

			const Patch& operator[](int) const noexcept;

			Patch& operator[](int) noexcept;

			const Patch* getSelected() const noexcept;

			Patch* getSelected() noexcept;

			const size_t size() const noexcept;

			void mouseWheelMove(const Mouse&, const MouseWheel&) override;

			// next
			void nextPatch(bool);
		private:
			std::array<Patch, NumPatches> patches;
			Patch* selected;
			ScrollBar scrollBar;
			Int64 directorySize;
			String filterName, filterAuthor;
		};

		struct ButtonSavePatch :
			public Button
		{
			// u, name, author
			ButtonSavePatch(Utils&, const TextEditor&,
				const TextEditor&);
		};

		struct ButtonReveal :
			public Button
		{
			ButtonReveal(Utils&, Patches&);
		};

		struct Browser :
			public Comp
		{
			Browser(Utils&);

			void paint(Graphics&) override;

			// next
			void nextPatch(bool);

			const Patch* getSelectedPatch() const noexcept;

			Patch* getSelectedPatch() noexcept;

			void resized() override;
		protected:
			Label title;
			TextEditor editorAuthor, editorName;
			Patches patches;
			ButtonSavePatch saveButton;
			ButtonReveal revealButton;
			String nameText, authorText;
		};

		struct BrowserButton :
			public Button
		{
			enum cb { kPresetNameAni, kNumAnis };

			BrowserButton(Utils&, Browser&);
		};

		struct NextPatchButton :
			public Button
		{
			// u, browser, next
			NextPatchButton(Utils&, Browser&, bool);
		};
	}
}

/*
saveButton.onClick when file already exists, ask if overwrite ok
*/