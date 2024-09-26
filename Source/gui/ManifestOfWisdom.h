#pragma once
#include "TextEditor.h"

namespace gui
{
	struct ManifestOfWisdom :
		public Comp
	{
		ManifestOfWisdom(Utils& u) :
			Comp(u),
			title(u),
			editor(u),
			manifest(u),
			inspire(u),
			reveal(u),
			clear(u),
			paste(u)
		{
			setOpaque(true);

			layout.init
			(
				{ 1, 1, 1, 1, 1 },
				{ 1, 13, 1, 2 }
			);

			addAndMakeVisible(title);
			addAndMakeVisible(manifest);
			addAndMakeVisible(inspire);
			addAndMakeVisible(reveal);
			addAndMakeVisible(clear);
			addAndMakeVisible(paste);
			addAndMakeVisible(editor);

			makeTextLabel(title, "Manifest of Wisdom", font::nel(), Just::centred, CID::Txt, "This is the glorious manifest of wisdom!");
			makeTextButton(manifest, "Manifest", "Click here to manifest this wisdom in the manifest of wisdom!", CID::Interact);
			makeTextButton(inspire, "Inspire", "Get inspired by random wisdom from the manifest of wisdom!", CID::Interact);
			makeTextButton(reveal, "Reveal", "Reveal the sacret manifest of wisdom!", CID::Interact);
			makeTextButton(clear, "Clear", "Clear the wisdom to make space for more wisdom for the manifest of wisdom!", CID::Interact);
			makeTextButton(paste, "Paste", "Paste wisdom from the clipboard to manifest it in the manifest of wisdom!", CID::Interact);
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xff000000));
			const auto thicc = utils.thicc;
			const auto textBounds = layout(0, 1, 5, 1);
			const auto col1 = Colours::c(CID::Darken);
			const auto col2 = Colour(0xff000000);
			const auto pt1 = textBounds.getTopLeft();
			const auto pt2 = textBounds.getBottomLeft();
			Gradient gradient(col1, pt1, col2, pt2, false);
			g.setGradientFill(gradient);
			g.fillRoundedRectangle(textBounds.reduced(thicc), thicc);
		}

		void resized() override
		{
			layout.resized(getLocalBounds());
			const auto thicc = utils.thicc;
			title.setBounds(layout.top().toNearestInt());
			title.setMaxHeight(thicc);
			layout.place(editor, 0, 1, 5, 1);
			layout.place(manifest, 0, 3, 1, 1);
			layout.place(inspire, 1, 3, 1, 1);
			layout.place(reveal, 2, 3, 1, 1);
			layout.place(clear, 3, 3, 1, 1);
			layout.place(paste, 4, 3, 1, 1);
		}

		Label title;
		TextEditor editor;
		Button manifest, inspire, reveal, clear, paste;
	};
}