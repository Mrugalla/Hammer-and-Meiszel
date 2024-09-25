#pragma once
#include "Button.h"

namespace gui
{
	struct ButtonColours :
		public Button
	{
		ButtonColours(Utils& u) :
			Button(u),
			img()
		{
			value = 0.f;
			makePaintButton(*this, [&](Graphics& g, const Button&)
				{
					if (!img.isValid())
						return;

					const auto phaseHover = callbacks[kHoverAniCB].phase;

					const auto w = static_cast<float>(img.getWidth());
					const auto h = static_cast<float>(img.getHeight());
					const auto maniSizeRel = Pi * .25f;
					const auto wNoHover = w * maniSizeRel;
					const auto hNoHover = h * maniSizeRel;
					const auto x = wNoHover + phaseHover * (w - wNoHover);
					const auto y = hNoHover + phaseHover * (h - hNoHover);

					g.drawImage
					(
						img.rescaled(static_cast<int>(x), static_cast<int>(y), ResamplingQuality::lowResamplingQuality),
						getLocalBounds().toFloat(),
						RectanglePlacement::doNotResize,
						false
					);
				}
			, "Click here to customize the visual style of the plugin.");
			notify(evt::Type::TooltipUpdated, &tooltip);
			onClick = [&](const Mouse&)
			{
				value = std::round(1.f - value);
			};
		}

	private:
		Image img;

		void resized() override
		{
			img = Image(Image::PixelFormat::ARGB, getWidth(), getHeight(), true);
			fixStupidJUCEImageThingie(img);

			const auto bounds = maxQuadIn(getLocalBounds().toFloat());
			const auto x = bounds.getX();
			const auto y = bounds.getY();
			const auto w = bounds.getWidth();
			const auto rad = w * .5f;
			const auto radHalf = rad * .5f;
			const PointF centre(x + rad, y + rad);
			for (auto i = 0; i < 3; ++i)
			{
				const auto iR = i * .33333f;
				const auto col = Colour::fromHSV(iR, 1.f, 1.f, 1.f);
				const auto angle = iR * Tau;
				const auto lineToCircle = LineF::fromStartAndAngle(centre, radHalf * Pi * .25f, angle);
				const auto circleCentre = lineToCircle.getEnd();
				for (auto j = 0; j < img.getHeight(); ++j)
				{
					const auto jF = static_cast<float>(j);
					for (auto k = 0; k < img.getWidth(); ++k)
					{
						const auto kF = static_cast<float>(k);
						const PointF pt(kF, jF);
						if (LineF(circleCentre, pt).getLength() < radHalf)
						{
							const auto pxl = img.getPixelAt(k, j);
							if(pxl.isTransparent())
								img.setPixelAt(k, j, col);
							else
								img.setPixelAt(k, j, pxl.interpolatedWith(col, .5f));
						}
							
					}
				}
			}
		}
	};
}