#pragma once
#include "Button.h"

namespace gui
{
	class Credits :
		public Comp
	{
		enum class Mode { Image, Text };

		struct Page
		{
			const void* data;
			int size;
			String info;
			int idx;
			Mode mode;
		};

		struct ZoomImage :
			public Comp
		{
			ZoomImage(Utils& u) :
				Comp(u),
				img(),
				pos(),
				zoomFactor(1.f)
			{
			}

			void paint(Graphics& g) override
			{
				if (!img.isValid())
					return;
				const auto bounds = getLocalBounds().toFloat();
				setCol(g, CID::Darken);
				g.fillRect(bounds);
				if (!isMouseOverOrDragging())
					return g.drawImage(img, bounds, RectanglePlacement::Flags::xMid);
				const auto imgWidth = static_cast<float>(img.getWidth());
				const auto imgHeight = static_cast<float>(img.getHeight());
				const auto xZoom = imgWidth * pos.x;
				const auto yZoom = imgHeight * pos.y;
				const auto wZoom = imgWidth * zoomFactor;
				const auto hZoom = imgHeight * zoomFactor;
				const auto x = xZoom - wZoom * .5f;
				const auto y = yZoom - hZoom * .5f;
				const BoundsF areaZoom(x, y, wZoom, hZoom);
				const auto imgZoom = img.getClippedImage(areaZoom.toNearestInt());
				g.drawImage(imgZoom, bounds, RectanglePlacement::Flags::xMid, false);
			}

			void init(const void* data, int size)
			{
				if (data == nullptr)
					return;
				img = ImageCache::getFromMemory(data, size);
			}

			void mouseEnter(const Mouse& mouse) override
			{
				updatePos(mouse.position);
			}

			void mouseMove(const Mouse& mouse) override
			{
				updatePos(mouse.position);
			}

			void mouseExit(const Mouse& mouse) override
			{
				updatePos(mouse.position);
			}

			void mouseDown(const Mouse&) override
			{
				updateZoom(.3f);
			}

			void mouseDrag(const Mouse& mouse) override
			{
				updatePos(mouse.position);
			}

			void mouseUp(const Mouse&) override
			{
				updateZoom(.5f);
			}

		private:
			Image img;
			PointF pos;
			float zoomFactor;

			void updatePos(const PointF& pt)
			{
				pos = pt;
				pos.x /= static_cast<float>(getWidth());
				pos.y /= static_cast<float>(getHeight());
				repaint();
			}

			void updateZoom(float factor)
			{
				zoomFactor = factor;
				repaint();
			}
		};

		struct Entry :
			public Comp
		{
			Entry(Utils& u) :
				Comp(u),
				img(u),
				info(u),
				idx(u),
				mode(Mode::Image)
			{
				layout.init
				(
					{ 13, 1 },
					{ 8, 3 }
				);
				addAndMakeVisible(img);
				addAndMakeVisible(info);
				addAndMakeVisible(idx);
				makeTextLabel(info, "", font::dosisMedium(), Just::topLeft, CID::Txt);
				makeTextLabel(idx, "", font::dosisMedium(), Just::centred, CID::Hover);
				info.autoMaxHeight = true;
				info.autoMaxHeight = true;
			}

			void init(const Page& page, int size)
			{
				mode = page.mode;
				if (page.data == nullptr)
					img.setVisible(false);
				else
				{
					img.setVisible(true);
					img.init(page.data, page.size);
				}
				info.setText(page.info);
				idx.setText(String(page.idx + 1) + " / " + String(size));
			}

			void resized() override
			{
				layout.resized(getLocalBounds());
				switch (mode)
				{
				case Mode::Image:
					layout.place(img, 0, 0, 2, 1);
					layout.place(info, 0, 1, 1, 1);
					break;
				case Mode::Text:
					layout.place(info, 0, 0, 2, 2);
					break;
				}
				layout.place(idx, 1, 1.7f, 1, .3f);
				info.setMaxHeight(utils.thicc);
			}

		private:
			ZoomImage img;
			Label info, idx;
			Mode mode;
		};

	public:
		Credits(Utils& u) :
			Comp(u),
			pages(),
			titleLabel(u),
			previous(u),
			next(u),
			entry(u),
			idx(0)
		{
			layout.init
			(
				{ 1, 21, 1 },
				{ 1, 13 }
			);

			addAndMakeVisible(titleLabel);
			addAndMakeVisible(previous);
			addAndMakeVisible(next);
			addAndMakeVisible(entry);

			makeTextLabel(titleLabel, "Credits", font::nel(), Just::centred, CID::Txt);
			makeTextButton(previous, "<", "Click here to look at the previous page.", CID::Interact);
			makeTextButton(next, ">", "Click here to look at the next page.", CID::Interact);

			previous.onClick = [&](const Mouse&)
			{
				if (idx <= 0)
					return;
				--idx;
				flipPage();
			};
			next.onClick = [&](const Mouse&)
			{
				if (idx >= pages.size() - 1)
					return;
				++idx;
				flipPage();
			};

			setOpaque(true);
		}

		void add(const void* data, int size, const String& info)
		{
			pages.push_back({ data, size, info, static_cast<int>(pages.size()), Mode::Image });
		}

		void add(const String& info)
		{
			pages.push_back({ nullptr, 0, info, static_cast<int>(pages.size()), Mode::Text });
		}

		void init()
		{
			idx = 0;
			entry.init(pages[idx], static_cast<int>(pages.size()));
		}

		void paint(Graphics& g) override
		{
			g.fillAll(getColour(CID::Bg));
		}

		void resized() override
		{
			layout.resized(getLocalBounds());
			layout.place(titleLabel, 1, 0, 1, 1);
			layout.place(previous, 0, 1, 1, 1);
			layout.place(next, 2, 1, 1, 1);
			layout.place(entry, 1, 1, 1, 1);
		}
	private:
		std::vector<Page> pages;
		Label titleLabel;
		Button previous, next;
		Entry entry;
		int idx;

		void flipPage()
		{
			entry.init(pages[idx], static_cast<int>(pages.size()));
			entry.resized();
			entry.repaint();
		}
	};

	struct ButtonCredits :
		public Button
	{
		ButtonCredits(Utils& u, Credits& credits) :
			Button(u)
		{
			type = Button::Type::kToggle;
			makeTextButton(*this, "Credits", "Click here to check out the credits and stuff! :)", CID::Interact);
			onClick = [&](const Mouse&)
			{
				auto e = !credits.isVisible();
				value = e ? 1.f : 0.f;
				credits.setVisible(e);
			};
			value = credits.isVisible() ? 1.f : 0.f;
		}
	};
}