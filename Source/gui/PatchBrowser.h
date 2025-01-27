#pragma once
#include "TextEditor.h"
#include "../arch/State.h"

namespace gui
{
	namespace patch
	{
		static constexpr int NumPatches = 12;

		inline File getPatchesDirectory(const Utils& u)
		{
			const auto& user = *u.audioProcessor.state.props.getUserSettings();
			const auto& settingsFile = user.getFile();
			const auto settingsDirectory = settingsFile.getParentDirectory();
			return settingsDirectory.getChildFile("Patches");
		}

		inline Int64 getDirectorySize(const File& directory)
		{
			const auto wildcard = "*.txt";
			const auto type = File::TypesOfFileToFind::findFiles;
			const RangedDirectoryIterator iterator
			(
				directory,
				true,
				wildcard,
				type
			);
			
			Int64 size = 0;
			for (const auto& it : iterator)
				size += it.getFileSize();
			return size;
		}

		struct Patch :
			public Comp
		{
			Patch(Utils& u) :
				Comp(u),
				name(""),
				author(""),
				file(),
				buttonLoad(u),
				buttonDelete(u)
			{
				layout.init
				(
					{ 8, 1 },
					{ 1 }
				);
				addAndMakeVisible(buttonLoad);
				addAndMakeVisible(buttonDelete);

				setInterceptsMouseClicks(false, true);
			}

			void activate(const String& _name, const String& _author, const File& _file)
			{
				name = _name;
				author = _author;
				file = _file;
				makeTextButton(buttonLoad, name + " by " + author, "Click here to load " + name + ".", CID::Interact);
				buttonLoad.label.font = font::flx();
				makePaintButton(buttonDelete, [](Graphics& g, const Button& b)
				{
					const auto thicc = b.utils.thicc;
					const auto thicc3 = thicc * 3.f;

					const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;

					const auto margin = thicc + thicc3 - hoverPhase * thicc3;
					const auto bounds = maxQuadIn(b.getLocalBounds().toFloat()).reduced(margin);
					
					const auto shrink = .2f;

					const auto w = bounds.getWidth();
					const auto wShrinked = hoverPhase * w * shrink;
					const auto h = bounds.getHeight();
					const auto y = bounds.getY();
					const auto x = bounds.getX() + wShrinked;
					const auto r = bounds.getRight() - wShrinked;
					
					setCol(g, CID::Interact);
					const auto lineThicc = thicc + hoverPhase * thicc;
					g.drawLine(x, y, r, y + h, lineThicc);
					g.drawLine(x, y + h, r, y, lineThicc);
				}, "Click here to remove " + name + ".");
				setVisible(true);
			}

			void deactivate()
			{
				setVisible(false);
			}

			void resized() override
			{
				layout.resized(getLocalBounds().toFloat());
				layout.place(buttonLoad, 0, 0, 1, 1);
				buttonLoad.label.setMaxHeight(utils.thicc);
				layout.place(buttonDelete, 1, 0, 1, 1);
			}

			String name, author;
			File file;
			Button buttonLoad, buttonDelete;
		};

		struct ScrollBar :
			public Comp
		{
			ScrollBar(Utils& u) :
				Comp(u),
				onScroll(),
				viewIdx(0),
				numFiles(1)
			{
			}

			void paint(Graphics& g) override
			{
				g.fillAll(getColour(CID::Darken));
				const auto bounds = getLocalBounds().toFloat();
				const auto x = 0.f;
				const auto w = bounds.getWidth() * 1.2f;
				const auto h = bounds.getHeight() / static_cast<float>(numFiles);
				const auto y = h * static_cast<float>(viewIdx);
				g.setColour(getColour(CID::Hover));
				g.drawRect(x, y, w, h, utils.thicc);
			}

			void mouseWheelMove(const Mouse&, const MouseWheel& wheel) override
			{
				auto inc = wheel.deltaY > 0.f ? -1 : 1;
				if(wheel.isReversed)
					inc = -inc;
				viewIdx += inc;
				if (viewIdx < 0)
					viewIdx = 0;
				onScroll();
				repaint();
			}

			std::function<void()> onScroll;
			int viewIdx, numFiles;
		};

		struct Patches :
			public Comp
		{
			Patches(Utils& u) :
				Comp(u),
				patches
				{
					Patch(u), Patch(u), Patch(u), Patch(u),
					Patch(u), Patch(u), Patch(u), Patch(u),
					Patch(u), Patch(u), Patch(u), Patch(u)
				},
				selected(nullptr),
				scrollBar(u),
				directorySize(getDirectorySize(getPatchesDirectory(u)))
			{
				layout.init
				(
					{ 13, 1 },
					{ 1 }
				);

				for (auto& patch : patches)
					addChildComponent(patch);
				addAndMakeVisible(scrollBar);

				scrollBar.onScroll = [&]()
				{
					update();
					resized();
					repaint();
				};

				for (auto& patch : patches)
				{
					patch.buttonLoad.onWheel = [&](const Mouse& mouse, const MouseWheel& wheel)
					{
						scrollBar.mouseWheelMove(mouse, wheel);
					};
					patch.buttonDelete.onWheel = [&](const Mouse& mouse, const MouseWheel& wheel)
					{
						scrollBar.mouseWheelMove(mouse, wheel);
					};
				}

				update();

				Comp::add(Callback([&]()
				{
					if (!isShowing())
						return;
					const auto patchesDirectory = getPatchesDirectory(u);
					const auto nSize = getDirectorySize(patchesDirectory);
					if (directorySize == nSize)
						return;
					directorySize = nSize;
					update();
					resized();
					repaint();
				}, 0, cbFPS::k_1_875, true));
			}

			void resized() override
			{
				layout.resized(getLocalBounds().toFloat());

				const auto w = layout.getW(0);
				const auto h = layout.getH(0);
				const auto patchHeight = h / static_cast<float>(NumPatches);
				auto patchY = 0.f;
				for (auto i = 0; i < NumPatches; ++i)
				{
					auto& patch = patches[i];
					patch.setBounds(BoundsF(0.f, patchY, w, patchHeight).toNearestInt());
					patch.resized();
					patchY += patchHeight;
				}

				layout.place(scrollBar, 1, 0, 1, 1);
			}

			void update()
			{
				const auto patchesDirectory = getPatchesDirectory(utils);
				const auto wildcard = "*.txt";
				const auto type = File::TypesOfFileToFind::findFiles;
				const RangedDirectoryIterator iterator
				(
					patchesDirectory,
					true,
					wildcard,
					type
				);

				const auto numFiles = patchesDirectory.getNumberOfChildFiles(type, wildcard);
				if (scrollBar.numFiles != numFiles)
				{
					scrollBar.numFiles = numFiles;
					scrollBar.repaint();
				}
				if(scrollBar.viewIdx >= scrollBar.numFiles)
					scrollBar.viewIdx = scrollBar.numFiles - 1;
				
				auto idx = -scrollBar.viewIdx;
				for (const auto& it : iterator)
				{
					if (idx >= 0)
					{
						if (idx < NumPatches)
						{
							const auto file = it.getFile();
							updateAdd(file, idx);
						}
					}
					++idx;
				}
				for (; idx < NumPatches; ++idx)
					patches[idx].deactivate();
			}

			void updateAdd(const File& file, int i)
			{
				const auto state = ValueTree::fromXml(file.loadFileAsString());
				auto name = file.getFileName();
				name = name.substring(0, name.lastIndexOf("."));
				const auto author = state.getProperty("author", "");

				patches[i].activate(name, author, file);
				patches[i].buttonLoad.onClick = [&, i](const Mouse&)
				{
					selected = &patches[i];
					const auto& file = patches[i].file;
					const auto vt = ValueTree::fromXml(file.loadFileAsString());
					if (!vt.isValid())
						return;
					auto& state = utils.audioProcessor.state;
					state.state = vt;
					utils.audioProcessor.params.loadPatch(state);
					utils.audioProcessor.pluginProcessor.loadPatch(state);
				};
				patches[i].buttonDelete.onClick = [&, i](const Mouse&)
				{
					if (patches[i].author == "factory")
						return;
					const auto file = patches[i].file;
					file.deleteFile();
					const auto directory = getPatchesDirectory(utils);
					directorySize = getDirectorySize(directory);
					patches[i].deactivate();
					update();
					resized();
					repaint();
				};
			}

			const Patch& operator[](int i) const noexcept
			{
				return patches[i];
			}

			Patch& operator[](int i) noexcept
			{
				return patches[i];
			}

			const Patch* getSelected() const noexcept
			{
				return selected;
			}

			Patch* getSelected() noexcept
			{
				return selected;
			}

			const size_t size() const noexcept
			{
				return patches.size();
			}

			void mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel) override
			{
				scrollBar.mouseWheelMove(mouse, wheel);
			}
		private:
			std::array<Patch, NumPatches> patches;
			Patch* selected;
			ScrollBar scrollBar;
			Int64 directorySize;
		};

		struct ButtonSavePatch :
			public Button
		{
			ButtonSavePatch(Utils& u, const TextEditor& editorName,
				const TextEditor& editorAuthor) :
				Button(u)
			{
				onClick = [&, &eName = editorName, &eAuthor = editorAuthor](const Mouse&)
				{
					const auto name = eName.txt;
					if (name.isEmpty())
						return;
					auto author = eAuthor.txt;
					if (author.isEmpty())
						author = "Audio Traveller";
					else if (author == "factory")
						return;
					const auto patchesDirectory = getPatchesDirectory(utils);
					auto& state = utils.audioProcessor.state;
					utils.audioProcessor.params.savePatch(state);
					utils.audioProcessor.pluginProcessor.savePatch(state);
					auto& vt = state.state;
					vt.setProperty("author", author, nullptr);
					const auto file = patchesDirectory.getChildFile(name + ".txt");
					if (file.existsAsFile())
						file.deleteFile();
					const auto result = file.create();
					if (result.failed())
						return;
					file.replaceWithText(vt.toXmlString());
				};

				makePaintButton(*this, [](Graphics& g, const Button& b)
				{
					const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
					const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;

					const auto thicc = b.utils.thicc;
					const auto lineThicc = thicc + hoverPhase + clickPhase * thicc;
					const auto bounds = maxQuadIn(b.getLocalBounds()).reduced(lineThicc);

					Path path;
					const auto x = bounds.getX();
					const auto y = bounds.getY();
					const auto width = bounds.getWidth();
					const auto height = bounds.getHeight();
					const auto btm = bounds.getBottom();
					const auto right = bounds.getRight();

					const auto x2 = x + width * .2f;
					const auto x8 = x + width * .8f;
					const auto y2 = y + height * .2f;
					const auto y4 = y + height * .4f;
					const auto y5 = y + height * .5f;
					const auto y4To5 = y4 + hoverPhase * (y5 - y4);

					path.startNewSubPath(bounds.getTopLeft());
					path.lineTo(x, btm);
					path.lineTo(right, btm);
					path.lineTo(right, y2);
					path.lineTo(x8, y);
					path.closeSubPath();

					path.startNewSubPath(x2, btm);
					path.lineTo(x2, y4To5);
					path.lineTo(x8, y4To5);
					path.lineTo(x8, btm);

					Stroke stroke(lineThicc, Stroke::JointStyle::beveled, Stroke::EndCapStyle::butt);
					setCol(g, CID::Interact);
					g.strokePath(path, stroke);
				}, "Click here to save this patch.");
			}
		};

		struct ButtonReveal :
			public Button
		{
			ButtonReveal(Utils& u, Patches& patches) :
				Button(u)
			{
				onClick = [&, &p = patches](const Mouse&)
				{
					auto patchSelected = p.getSelected();
					if (patchSelected)
					{
						const auto& file = patchSelected->file;
						file.revealToUser();
						return;
					}
					const auto patchesDirectory = getPatchesDirectory(utils);
					patchesDirectory.revealToUser();
				};

				makePaintButton(*this, [](Graphics& g, const Button& b)
				{
					const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
					const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;

					const auto thicc = b.utils.thicc;
					const auto lineThicc = thicc + hoverPhase + clickPhase * thicc;

					const auto bounds = maxQuadIn(b.getLocalBounds()).reduced(lineThicc);

					const auto x = bounds.getX();
					const auto y = bounds.getY();
					const auto w = bounds.getWidth();
					const auto h = bounds.getHeight();
					const auto r = x + w;
					const auto btm = y + h;

					Path path;
					const auto x1 = x + w * .1f;
					const auto x2 = x + w * .2f;
					const auto x4 = x + w * .4f;
					const auto x5 = x + w * .5f;
					const auto x8 = x + w * .8f;
					const auto x9 = x + w * .9f;
					const auto y1 = y + h * .1f;
					const auto y2 = y + h * .2f;
					const auto y3 = y + h * .3f;
					const auto y5 = y + h * .5f;
					const auto y3To5 = y3 + hoverPhase * (y5 - y3);
					path.startNewSubPath(x1, y);
					path.lineTo(x4, y);
					path.lineTo(x5, y1);
					path.lineTo(x9, y1);
					path.lineTo(x8, y3To5);
					path.lineTo(x2, y3To5);
					path.lineTo(x, btm);
					path.lineTo(x, y2);
					path.closeSubPath();

					path.startNewSubPath(x, btm);
					path.lineTo(x8, btm);
					path.lineTo(r, y3To5);
					path.lineTo(x8, y3To5);

					Stroke stroke(lineThicc, Stroke::JointStyle::beveled, Stroke::EndCapStyle::butt);
					setCol(g, CID::Interact);
					g.strokePath(path, stroke);
				}, "Click here to reveal the patches directory or the selected patch.");
			}
		};

		struct Browser :
			public Comp
		{
			Browser(Utils& u) :
				Comp(u),
				title(u),
				editorAuthor(u, "enter author"),
				editorName(u, "enter name"),
				patches(u),
				saveButton(u, editorName, editorAuthor),
				revealButton(u, patches)
			{
				layout.init
				(
					{ 2, 2, 1, 1 },
					{ 1, 1, 13 }
				);

				addAndMakeVisible(title);
				addAndMakeVisible(editorAuthor);
				addAndMakeVisible(editorName);
				addAndMakeVisible(saveButton);
				addAndMakeVisible(revealButton);
				addAndMakeVisible(patches);

				editorAuthor.tooltip = "Click here to enter the name of the author of the current patch.";
				editorName.tooltip = "Click here to enter the name of the current patch.";
				editorAuthor.labelEmpty.setText("Author");
				editorName.labelEmpty.setText("Name");

				makeTextLabel
				(
					title,
					"Patch Browser",
					font::dosisLight(),
					Just::centred, CID::Txt,
					"You have entered the patch browser. no shit."
				);
				title.autoMaxHeight = true;
			}

			void paint(Graphics& g) override
			{
				g.fillAll(getColour(CID::Bg));
			}

			const Patch* getSelectedPatch() const noexcept
			{
				return patches.getSelected();
			}

			Patch* getSelectedPatch() noexcept
			{
				return patches.getSelected();
			}

			void resized() override
			{
				layout.resized(getLocalBounds().toFloat());
				layout.place(title, 0, 0, 4, 1);
				layout.place(editorName, 0, 1, 1, 1);
				layout.place(editorAuthor, 1, 1, 1, 1);
				layout.place(saveButton, 2, 1, 1, 1);
				layout.place(revealButton, 3, 1, 1, 1);
				layout.place(patches, 0, 2, 4, 1);
			}
		protected:
			Label title;
			TextEditor editorAuthor, editorName;
			Patches patches;
			ButtonSavePatch saveButton;
			ButtonReveal revealButton;
		};

		struct BrowserButton :
			public Button
		{
			enum cb { kPresetNameAni, kNumAnis };

			BrowserButton(Utils& u, Browser& browser) :
				Button(u)
			{
				makeTextButton(*this, "Patches", "Click here to save, browse or manage patches.", CID::Interact);
				label.autoMaxHeight = true;
				onClick = [&b = browser](const Mouse&)
				{
					b.setVisible(!b.isVisible());
				};
			}
		};
	}
}

/*
saveButton.onClick when file already exists, ask if overwrite ok
*/