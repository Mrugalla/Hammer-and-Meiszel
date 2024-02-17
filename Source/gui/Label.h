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
		Label(Utils&, bool = true);

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