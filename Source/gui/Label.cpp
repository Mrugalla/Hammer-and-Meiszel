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

	Toast::Toast(Utils& u) :
		Label(u, true),
		events(),
		alphaAniWeight(false)
	{
		makeTextLabel(*this, "", font::dosisBold(), Just::centred, CID::Txt);

		const auto fpsToast = cbFPS::k30;
		const auto speedIn = msToInc(AniLengthMs, fpsToast);
		const auto speedOut = msToInc(AniLengthMs * 2.f, fpsToast);
		add(Callback([&, speedIn, speedOut]()
		{
			auto& phase = callbacks[kCBFade].phase;
			if (alphaAniWeight)
			{
				if (phase == 1.f)
					return;
				phase += speedIn;
				if (phase >= 1.f)
					callbacks[kCBFade].stop(1.f);
			}
			else
			{
				if (phase == 0.f)
					return;
				phase -= speedOut;
				if (phase <= 0.f)
				{
					callbacks[kCBFade].stop(0.f);
					return setVisible(false);
				}
			}
			setAlpha(math::tanhApprox(2.f * phase));
			setVisible(true);
		}, kCBFade, fpsToast, false));

		addEvt([&](const evt::Type t, const void* stuff)
		{
			if (t == evt::Type::ToastShowUp)
			{
				const auto bounds = *static_cast<const Bounds*>(stuff);
				const Point offset(getWidth(), 0);
				const auto y = bounds.getTopLeft().y;
				const auto nX = bounds.getTopLeft().x - offset.x;
				if (nX > getWidth())
					setTopLeftPosition(nX, y);
				else
					setTopLeftPosition(bounds.getTopLeft().x + getWidth(), y);
				alphaAniWeight = true;
				callbacks[kCBFade].start(callbacks[kCBFade].phase);
			}
			else if (t == evt::Type::ToastUpdateMessage)
			{
				const auto nMessage = *static_cast<const String*>(stuff);
				setText(nMessage);
				repaint();
			}
			else if (t == evt::Type::ToastVanish)
			{
				alphaAniWeight = false;
				callbacks[kCBFade].start(callbacks[kCBFade].phase);
			}
			else if (t == evt::Type::ToastColour)
			{
				const auto cID = *static_cast<const CID*>(stuff);
				makeTextLabel(*this, "", font::dosisBold(), Just::centred, cID);
			}
		});
	}

	void Toast::paint(Graphics& g)
	{
		setCol(g, CID::Darken);
		const auto t2 = utils.thicc;
		g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(t2), t2);
		Label::paint(g);
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

	//////

	LabelGroup::LabelGroup() :
		labels()
	{
	}

	void LabelGroup::clear()
	{
		labels.clear();
	}

	void LabelGroup::add(Label& label)
	{
		labels.push_back(&label);
	}

	void LabelGroup::setMaxHeight(float margin) noexcept
	{
		const auto thicc = labels[0]->utils.thicc;
		const auto h = findMaxCommonHeight(labels) - margin;
		const auto height = h < 0.f ? thicc : h;
		for (auto i = 0; i < labels.size(); ++i)
			labels[i]->setHeight(height);
	}

	//////
}