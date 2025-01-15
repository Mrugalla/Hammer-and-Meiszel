#include "Label.h"

namespace gui
{
	Label::Label(Utils& u, bool _autoMaxHeight) :
		Comp(u),
		text(""),
		font(FontOptions()),
		just(Just::centred),
		onPaint([](Graphics&, const Label&) {}),
		img(),
		cID(CID::Txt),
		type(Type::NumTypes),
		autoMaxHeight(_autoMaxHeight)
	{
		setInterceptsMouseClicks(false, false);
	}

	void Label::resized()
	{
		if (autoMaxHeight)
			setMaxHeight();
	}

	bool Label::isEmpty() const noexcept
	{
		return text.isEmpty();
	}

	bool Label::isNotEmpty() const noexcept
	{
		return !isEmpty();
	}

	void Label::setText(const String& txt)
	{
		if (txt == text)
			return;

		text = txt;

		if (autoMaxHeight)
			setMaxHeight();
	}
	
	void Label::replaceSpacesWithLineBreaks()
	{
		text = text.replaceCharacters(" ", "\n");
	}

	void Label::paint(Graphics& g)
	{
		switch (type)
		{
		case Type::Text:
			g.setFont(font);
			g.setColour(getColour(cID));
			g.drawFittedText(text, getLocalBounds(), just, 1);
			break;
		case Type::Paint:
			onPaint(g, *this);
			break;
		case Type::Image:
			g.drawImage(img, getLocalBounds().toFloat(), RectPlacement::Flags::centred, false);
			break;
		}
	}

	void Label::setHeight(float h) noexcept
	{
		if (h == 0.f)
			return;
		font.setHeight(h);
	}

	float Label::getMaxHeight() const noexcept
	{
		const auto w = static_cast<float>(getWidth());
		const auto h = static_cast<float>(getHeight());
		return findMaxHeight(font, text, w, h);
	}

	void Label::setMaxHeight(float margin) noexcept
	{
		const auto thicc = utils.thicc;
		const auto h = getMaxHeight() - margin;
		setHeight(h < thicc ? thicc : h);
	}

	//////

	void makeTextLabel(Label& label, const String& text, const Font& font, Just just, CID cID, const String& tooltip)
	{
		label.type = Label::Type::Text;
		label.setText(text);
		label.font = font;
		label.just = just;
		label.cID = cID;
		label.setTooltip(tooltip);
	}

	void makePaintLabel(Label& label, const Label::OnPaint& onPaint, const String& tooltip)
	{
		label.type = Label::Type::Paint;
		label.onPaint = onPaint;
		label.setTooltip(tooltip);
	}

	void makeImageLabel(Label& label, const Image& img, const String& tooltip)
	{
		label.type = Label::Type::Image;
		label.img = img;
		label.setTooltip(tooltip);
	}

	//////

	float findMaxCommonHeight(const Label* labels, int size) noexcept
	{
		auto maxHeight = labels[0].getMaxHeight();
		for (auto i = 1; i < size; ++i)
			maxHeight = std::min(labels[i].getMaxHeight(), maxHeight);
		return maxHeight;
	}

	float findMaxCommonHeight(const std::vector<Label*>& labels) noexcept
	{
		auto maxHeight = labels[0]->getMaxHeight();
		for (auto i = 1; i < labels.size(); ++i)
			maxHeight = std::min(labels[i]->getMaxHeight(), maxHeight);
		return maxHeight;
	}

	void setMaxCommonHeight(Label* labels, int size) noexcept
	{
		const auto h = findMaxCommonHeight(labels, size);
		for (auto i = 0; i < size; ++i)
			labels[i].setHeight(h);
	}

	void setMaxCommonHeight(Label* labels, size_t size) noexcept
	{
		setMaxCommonHeight(labels, static_cast<int>(size));
	}
}