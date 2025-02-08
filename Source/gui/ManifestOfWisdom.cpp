#include "ManifestOfWisdom.h"

namespace gui
{
	String getFolder()
	{
		const auto slash = File::getSeparatorString();
		const auto specialLoc = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);
		return specialLoc.getFullPathName() + slash + "Mrugalla" + slash + "SharedState" + slash + "TheManifestOfWisdom" + slash;
	}

	// ManifestOfWisdom

	ManifestOfWisdom::ManifestOfWisdom(Utils& u) :
		Comp(u),
		title(u),
		subTitle(u),
		alert(u),
		editor(u),
		manifest(u),
		inspire(u),
		reveal(u),
		clear(u),
		paste(u),
		buttonLabelsGroup()
	{
		setOpaque(true);

		layout.init
		(
			{ 1, 1, 1, 1, 1 },
			{ 1, 3, 13, 1, 1 }
		);

		addAndMakeVisible(title);
		addAndMakeVisible(subTitle);
		addAndMakeVisible(alert);
		addAndMakeVisible(manifest);
		addAndMakeVisible(inspire);
		addAndMakeVisible(reveal);
		addAndMakeVisible(clear);
		addAndMakeVisible(paste);
		addAndMakeVisible(editor);

		editor.onClick = [&](const Mouse&)
		{
			editor.setActive(true);
		};

		manifest.onClick = [&](const Mouse&)
		{
			if (editor.isEmpty())
			{
				Random rand;
				auto r = rand.nextInt(3);
				switch (r)
				{
				case 0: return parse("A shut mouth catches no flies.");
				case 1: return parse("I admire your meditation. unfortunately wisdom can not be empty.");
				case 2: return parse("Try to enter wisdom before manifesting it. (This feature has no lookahead)");
				}
			}

			const auto now = Time::getCurrentTime();
			const auto year = now.getYear();
			const auto month = now.getMonth();
			const auto day = now.getDayOfMonth();
			const auto hour = now.getHours();
			const auto minute = now.getMinutes();
			const auto sec = now.getSeconds();
			String fileName;
			fileName += String(year) + "_";
			fileName += String(month < 10 ? "0" : "") + String(month) + "_";
			fileName += String(day < 10 ? "0" : "") + String(day) + "_";
			fileName += String(hour < 10 ? "0" : "") + String(hour) + "_";
			fileName += String(minute < 10 ? "0" : "") + String(minute) + "_";
			fileName += String(sec < 10 ? "0" : "") + String(sec) + ".txt";
			File file(getFolder() + fileName);
			if (file.existsAsFile())
				return;
			file.create();
			file.appendText(editor.txt);
			file.revealToUser();
			editor.setActive(false);
			parse("Manifested: " + fileName);
		};

		inspire.onClick = [&](const Mouse&)
		{
			const File folder(getFolder());
			const auto fileTypes = File::TypesOfFileToFind::findFiles;
			const String extension(".txt");
			const auto wildCard = "*" + extension;
			const RangedDirectoryIterator files
			(
				folder,
				false,
				wildCard,
				fileTypes
			);

			const auto numFiles = folder.getNumberOfChildFiles(fileTypes, wildCard);
			if (numFiles == 0)
			{
				editor.clear();
				editor.setActive(true);
				Random rand;
				auto r = rand.nextInt(3);
				switch (r)
				{
				case 0: return parse("Apologies! There is no wisdom to be found in the manifest of wisdom yet..");
				case 1: return parse("You must have entered arcane wisdom in the past for it to show up now.");
				case 2: return parse("The manifest of wisdom is a book that is yet to be written.");
				}
			}

			Random rand;
			auto idx = rand.nextInt(numFiles);
			for (const auto& it : files)
			{
				if (idx == 0)
				{
					const File file(it.getFile());
					parse(file.getFileName());
					editor.setText(file.loadFileAsString());
					editor.setActive(false);
					return;
				}
				else
					--idx;
			}
		};

		reveal.onClick = [&](const Mouse&)
		{
			const File file(getFolder() + alert.text);
			if (file.exists())
				return file.revealToUser();
			const File folder(getFolder());
			folder.revealToUser();
		};

		clear.onClick = [&](const Mouse&)
		{
			editor.clear();
			editor.setActive(true);
			parse();
		};

		paste.onClick = [&](const Mouse&)
		{
			editor.paste();
			editor.setActive(true);
		};

		makeTextLabel
		(
			title,
			"Manifest of Wisdom",
			font::nel(), Just::centred, CID::Txt,
			"This is the glorious manifest of wisdom!"
		);
		makeTextLabel
		(
			subTitle,
			"Manifest your wisdom in the manifest of wisdom\nor get inspired by arcane wisdom from the manifest of wisdom!",
			font::flx(), Just::centred, CID::Hover,
			"The manifest of wisdom grants you access to arcane wisdom in most of my plugins!"
		);
		makeTextLabel(alert, "", font::dosisMedium(), Just::centred, CID::Hover);
		makeTextButton(manifest, "Manifest", "Click here to manifest this wisdom in the manifest of wisdom!", CID::Interact);
		makeTextButton(inspire, "Inspire", "Get inspired by random wisdom from the manifest of wisdom!", CID::Interact);
		makeTextButton(reveal, "Reveal", "Reveal the sacret manifest of wisdom!", CID::Interact);
		makeTextButton(clear, "Clear", "Clear the wisdom to make space for more wisdom for the manifest of wisdom!", CID::Interact);
		makeTextButton(paste, "Paste", "Paste wisdom from the clipboard to manifest it in the manifest of wisdom!", CID::Interact);

		buttonLabelsGroup.add(manifest.label);
		buttonLabelsGroup.add(inspire.label);
		buttonLabelsGroup.add(reveal.label);
		buttonLabelsGroup.add(clear.label);
		buttonLabelsGroup.add(paste.label);

		title.autoMaxHeight = false;
		subTitle.autoMaxHeight = false;
		manifest.label.autoMaxHeight = false;
		inspire.label.autoMaxHeight = false;
		reveal.label.autoMaxHeight = false;
		clear.label.autoMaxHeight = false;
		paste.label.autoMaxHeight = false;
	}

	void ManifestOfWisdom::paint(Graphics& g)
	{
		g.fillAll(Colour(0xff000000));
		const auto thicc = utils.thicc;
		const auto textBounds = layout(0, 1, 5, 2);
		const auto col1 = Colours::c(CID::Darken);
		const auto col2 = Colour(0xff000000);
		const auto pt1 = textBounds.getTopLeft();
		const auto pt2 = textBounds.getBottomLeft();
		Gradient gradient(col1, pt1, col2, pt2, false);
		g.setGradientFill(gradient);
		g.fillRoundedRectangle(textBounds.reduced(thicc), thicc);
	}

	void ManifestOfWisdom::resized()
	{
		layout.resized(getLocalBounds());
		const auto thicc = utils.thicc;
		title.setBounds(layout.top().toNearestInt());
		subTitle.setBounds(layout(0, 1, 5, 1).toNearestInt());
		subTitle.setMaxHeight(thicc * 2.f);
		title.setMaxHeight(thicc);
		layout.place(editor, 0, 2, 5, 1);
		layout.place(alert, 0, 3, 5, 1);
		layout.place(manifest, 0, 4, 1, 1);
		layout.place(inspire, 1, 4, 1, 1);
		layout.place(reveal, 2, 4, 1, 1);
		layout.place(clear, 3, 4, 1, 1);
		layout.place(paste, 4, 4, 1, 1);

		buttonLabelsGroup.setMaxHeight(thicc * 2.f);
	}

	void ManifestOfWisdom::parse(const String& msg)
	{
		alert.setText(msg);
		alert.setMaxHeight(utils.thicc);
		alert.repaint();
	}

	// ButtonWisdom

	ButtonWisdom::ButtonWisdom(Utils& u, ManifestOfWisdom& menu) :
		Button(u),
		book(ImageCache::getFromMemory(BinaryData::mow_png, BinaryData::mow_pngSize)),
		bookHover(book),
		bookX(0), bookY(0)
	{
		const auto paintVisor = makeButtonOnPaintVisor(2);

		onPaint = [&, paintVisor](Graphics& g, const Button& b)
		{
			paintVisor(g, b);
			g.drawImageAt(book, bookX, bookY, false);
		};

		setTooltip("Click here to manifest wisdom in the manifest of wisdom!");

		onClick = [&](const Mouse&)
		{
			const auto e = !menu.isVisible();
			if (e)
				utils.eventSystem.notify(evt::Type::ClickedEmpty);
			menu.setVisible(e);
			value = e ? 1.f : 0.f;
		};
	}

	void ButtonWisdom::resized()
	{
		const auto w = static_cast<float>(getWidth());
		const auto h = static_cast<float>(getHeight());

		const auto mow = ImageCache::getFromMemory(BinaryData::mow_png, BinaryData::mow_pngSize);
		const auto mowWidth = static_cast<float>(mow.getWidth());
		const auto mowHeight = static_cast<float>(mow.getHeight());

		const auto wRatio = w / mowWidth;
		const auto hRatio = h / mowHeight;

		if (wRatio > hRatio)
		{
			const auto nW = static_cast<int>(mowWidth * hRatio);
			bookX = (getWidth() - nW) / 2;
			bookY = 0;
			book = mow.rescaled(nW, getHeight(), Graphics::lowResamplingQuality);
		}
		else
		{
			const auto nH = static_cast<int>(mowHeight * wRatio);
			bookX = 0;
			bookY = (getHeight() - nH) / 2;
			book = mow.rescaled(getWidth(), nH, Graphics::lowResamplingQuality);
		}
	}
}