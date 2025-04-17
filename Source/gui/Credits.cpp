#include "Credits.h"

namespace gui
{
	// ZoomImage

	Credits::ZoomImage::ZoomImage(Utils& u) :
		Comp(u),
		img(),
		pos(),
		zoomFactor(1.f)
	{
	}

	void Credits::ZoomImage::paint(Graphics& g)
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

	void Credits::ZoomImage::init(const void* data, int size)
	{
		if (data == nullptr)
			return;
		img = ImageCache::getFromMemory(data, size);
	}

	void Credits::ZoomImage::mouseEnter(const Mouse& mouse)
	{
		updatePos(mouse.position);
	}

	void Credits::ZoomImage::mouseMove(const Mouse& mouse)
	{
		updatePos(mouse.position);
	}

	void Credits::ZoomImage::mouseExit(const Mouse& mouse)
	{
		updatePos(mouse.position);
	}

	void Credits::ZoomImage::mouseDown(const Mouse&)
	{
		updateZoom(.3f);
	}

	void Credits::ZoomImage::mouseDrag(const Mouse& mouse)
	{
		updatePos(mouse.position);
	}

	void Credits::ZoomImage::mouseUp(const Mouse&)
	{
		updateZoom(.5f);
	}

	void Credits::ZoomImage::updatePos(const PointF& pt)
	{
		pos = pt;
		pos.x /= static_cast<float>(getWidth());
		pos.y /= static_cast<float>(getHeight());
		repaint();
	}

	void Credits::ZoomImage::updateZoom(float factor)
	{
		zoomFactor = factor;
		repaint();
	}

	// LinksPage

	Credits::LinksPage::LinksPage(Utils& u) :
		Comp(u)
	{
	}

	void Credits::LinksPage::init(const Links& links)
	{
		clear();
		for (auto& link : links)
		{
			buttons.push_back(std::make_unique<ButtonLink>(utils));
			auto& button = *buttons.back();
			button.init(link.name, link.url);
		}
		for (auto& button : buttons)
			addAndMakeVisible(*button);
		resized();
		repaint();
	}

	void Credits::LinksPage::resized()
	{
		auto bounds = getLocalBounds().toFloat();
		const auto x = 0.f;
		const auto w = bounds.getWidth();
		const auto h = bounds.getHeight() / static_cast<float>(buttons.size());
		auto y = 0.f;
		for (auto& button : buttons)
		{
			button->setBounds(BoundsF(x, y, w, h).toNearestInt());
			y += h;
		}
	}

	void Credits::LinksPage::clear()
	{
		for (auto& button : buttons)
			removeChildComponent(button.get());
		buttons.clear();
	}

	// Entry

	Credits::Entry::Entry(Utils& u) :
		Comp(u),
		linksPage(u),
		img(u),
		info(u),
		idx(u),
		footer(u),
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
		addAndMakeVisible(footer);
		addAndMakeVisible(linksPage);
		makeTextLabel(info, "", font::dosisMedium(), Just::topLeft, CID::Txt);
		makeTextLabel(idx, "", font::dosisMedium(), Just::centred, CID::Hover);
		makeTextLabel(footer, "", font::dosisMedium(), Just::topLeft, CID::Txt);
		info.autoMaxHeight = true;
		idx.autoMaxHeight = true;
		footer.autoMaxHeight = true;
	}

	void Credits::Entry::init(const Page& page, int size)
	{
		mode = page.mode;
		if (page.data == nullptr)
			img.setVisible(false);
		else
		{
			img.setVisible(true);
			img.init(page.data, page.size);
		}
		linksPage.init(page.links);
		info.setText(page.info);
		footer.setText(page.footer);
		idx.setText(String(page.idx + 1) + " / " + String(size));
	}

	void Credits::Entry::resized()
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
		case Mode::Links:
			layout.place(info, 0, 0.f, 2, .15f);
			layout.place(linksPage, 0, .15f, 2, 1.55f);
			layout.place(footer, 0, 1.7f, 2, .3f);
			break;
		}
		layout.place(idx, 1, 1.7f, 1, .3f);
		info.setMaxHeight(utils.thicc);
	}

	// Credits

	Credits::Credits(Utils& u) :
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

	void Credits::add(const void* data, int size, const String& info)
	{
		pages.push_back({ data, size, info, static_cast<int>(pages.size()), {}, "", Mode::Image });
	}

	void Credits::add(const String& info)
	{
		pages.push_back({ nullptr, 0, info, static_cast<int>(pages.size()), {}, "", Mode::Text });
	}

	void Credits::add(const String& title, const Links& links, const String& subTitle)
	{
		pages.push_back({ nullptr, 0, title, static_cast<int>(pages.size()), links, subTitle, Mode::Links });
	}

	void Credits::init()
	{
		idx = 0;
		entry.init(pages[idx], static_cast<int>(pages.size()));
	}

	void Credits::paint(Graphics& g)
	{
		g.fillAll(getColour(CID::Bg));
	}

	void Credits::resized()
	{
		layout.resized(getLocalBounds());
		layout.place(titleLabel, 1, 0, 1, 1);
		layout.place(previous, 0, 1, 1, 1);
		layout.place(next, 2, 1, 1, 1);
		layout.place(entry, 1, 1, 1, 1);
	}

	void Credits::flipPage()
	{
		entry.init(pages[idx], static_cast<int>(pages.size()));
		entry.resized();
		entry.repaint();
	}

	// Button

	ButtonCredits::ButtonCredits(Utils& u, Credits& credits) :
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
}