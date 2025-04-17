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

		// u, autoMaxHeight
		Label(Utils&, bool = false);

		void resized() override;

		bool isEmpty() const noexcept;

		bool isNotEmpty() const noexcept;

		void setText(const String&);

		void replaceSpacesWithLineBreaks();

		void paint(Graphics&) override;

		void setHeight(float) noexcept;

		float getMaxHeight() const noexcept;

		// margin
		void setMaxHeight(float = 0.f) noexcept;

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

	// label, text, font, just, cID, tooltip
	void makeTextLabel(Label&, const String&, const Font&, Just, CID, const String& = "");

	// label, onPaint, tooltip
	void makePaintLabel(Label&, const Label::OnPaint&, const String& = "");

	// label, image, tooltip
	void makeImageLabel(Label&, const Image&, const String& = "");

	//////

	class Toast :
		public Label
	{
		enum kCBs { kCBFade, kNumCBs };
	public:
		Toast(Utils&);

		void paint(Graphics&) override;
		
		std::vector<std::function<void()>> events;
		bool alphaAniWeight;
	};

	// labels, size
	float findMaxCommonHeight(const Label*, int) noexcept;

	float findMaxCommonHeight(const std::vector<Label*>&) noexcept;

	// labels, size
	void setMaxCommonHeight(Label*, int) noexcept;

	// labels, size
	void setMaxCommonHeight(Label*, size_t) noexcept;
	
	struct LabelGroup
	{
		LabelGroup();

		void clear();

		void add(Label&);

		// margin
		void setMaxHeight(float = 0.f) noexcept;

	protected:
		std::vector<Label*> labels;
	};
}