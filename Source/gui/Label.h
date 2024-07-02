#pragma once
#include "Comp.h"

namespace gui
{
	struct Label :
		public Comp
	{
		using OnPaint = std::function<void(Graphics&, const Label&)>;

		enum class Type { Text, Paint, Image, NumTypes };
		static constexpr int NumTypes = static_cast<int>(Type::NumTypes);

		/* u, autoMaxHeight */
		Label(Utils&, bool = false);

		void resized() override;

		bool isEmpty() const noexcept;

		bool isNotEmpty() const noexcept;

		void setText(const String&);

		void replaceSpacesWithLineBreaks();

		void paint(Graphics&) override;

		void setHeight(float) noexcept;

		float getMaxHeight() const noexcept;

		void setMaxHeight() noexcept;

		String text;
		Font font;
		Just just;
		OnPaint onPaint;
		Image img;
		CID cID;
		Type type;
		bool autoMaxHeight;
	};

	//////

	/* label, text, font, just, cID, tooltip */
	void makeTextLabel(Label&, const String&, const Font&, Just, CID, const String& = "");

	/* label, onPaint, tooltip */
	void makePaintLabel(Label&, const Label::OnPaint&, const String& = "");

	/* label, image, tooltip */
	void makeImageLabel(Label&, const Image&, const String& = "");

	//////

	class Toast :
		public Label
	{
		enum kCBs { kCBFade, kNumCBs };
	public:
		Toast(Utils& u) :
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
					if(nX > getWidth())
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

		void paint(Graphics& g) override
		{
			setCol(g, CID::Darken);
			const auto t2 = utils.thicc * 2.f;
			g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(t2), t2);
			Label::paint(g);
		}
		
		std::vector<std::function<void()>> events;
		bool alphaAniWeight;
	};

	/* labels, size */
	float findMaxCommonHeight(const Label*, int) noexcept;

	float findMaxCommonHeight(const std::vector<Label*>&) noexcept;

	/* labels, size */
	void setMaxCommonHeight(Label*, int) noexcept;

	/* labels, size */
	void setMaxCommonHeight(Label*, size_t) noexcept;
	
	struct LabelGroup
	{
		LabelGroup() :
			labels()
		{}

		void add(Label& label)
		{
			labels.push_back(&label);
		}

		void setMaxHeight() noexcept
		{
			const auto h = findMaxCommonHeight(labels);
			for (auto i = 0; i < labels.size(); ++i)
				labels[i]->setHeight(h);
		}

	protected:
		std::vector<Label*> labels;
	};
}