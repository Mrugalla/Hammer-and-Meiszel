#include "Cursor.h"
#include "Layout.h"

namespace gui
{
	MouseCursor makeCursor()
	{
		Image img = juce::ImageCache::getFromMemory(BinaryData::cursor_png, BinaryData::cursor_pngSize).createCopy();
		fixStupidJUCEImageThingie(img);

		const auto w = img.getWidth();
		const auto h = img.getHeight();
		const Colour imgCol(0xff37946e);
		const auto col = getColour(CID::Interact);

		for (auto y = 0; y < h; ++y)
			for (auto x = 0; x < w; ++x)
				if (img.getPixelAt(x, y) == imgCol)
					img.setPixelAt(x, y, col);

		static constexpr int scale = 3;
		img = img.rescaled(w * scale, h * scale, Graphics::ResamplingQuality::lowResamplingQuality);

		return { img, 0, 0 };
	}
}