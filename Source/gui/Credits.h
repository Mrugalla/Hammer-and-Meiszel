#pragma once
#include "ButtonLink.h"

namespace gui
{
	struct Credits :
		public Comp
	{
		enum class Mode { Image, Text, Links };
		
		using Links = ButtonLink::Links;
	private:
		struct Page
		{
			const void* data;
			int size;
			String info;
			int idx;
			Links links;
			String footer;
			Mode mode;
		};

		struct ZoomImage :
			public Comp
		{
			ZoomImage(Utils&);

			void paint(Graphics&) override;

			// data, size
			void init(const void*, int);

			void mouseEnter(const Mouse&) override;

			void mouseMove(const Mouse&) override;

			void mouseExit(const Mouse&) override;

			void mouseDown(const Mouse&) override;

			void mouseDrag(const Mouse&) override;

			void mouseUp(const Mouse&) override;

		private:
			Image img;
			PointF pos;
			float zoomFactor;

			void updatePos(const PointF&);

			void updateZoom(float);
		};

		struct LinksPage :
			public Comp
		{
			LinksPage(Utils&);

			void init(const Links&);

			void resized() override;

		private:
			std::vector<std::unique_ptr<ButtonLink>> buttons;

			void clear();
		};

		struct Entry :
			public Comp
		{
			Entry(Utils&);

			void init(const Page&, int);

			void resized() override;

		private:
			LinksPage linksPage;
			ZoomImage img;
			Label info, idx, footer;
			Mode mode;
		};

	public:
		Credits(Utils&);

		// data, size, info
		void add(const void*, int, const String&);

		// text
		void add(const String&);

		// title, links, subTitle
		void add(const String&, const Links&, const String&);

		void init();

		void paint(Graphics&) override;

		void resized() override;
	private:
		std::vector<Page> pages;
		Label titleLabel;
		Button previous, next;
		Entry entry;
		int idx;

		void flipPage();
	};

	struct ButtonCredits :
		public Button
	{
		ButtonCredits(Utils&, Credits&);
	};
}