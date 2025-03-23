#include "VoiceGrid.h"

namespace gui
{
	template<size_t NumVoices>
	VoiceGrid<NumVoices>::VoiceGrid(Utils& u) :
		Comp(u),
		voices(),
		poly(NumVoices)
	{
	}
	
	template<size_t NumVoices>
	void VoiceGrid<NumVoices>::init(const UpdateFunc& updateFunc)
	{
		const auto fps = cbFPS::k30;
		add(Callback([&, wannaRepaint = updateFunc]()
		{
			if (wannaRepaint(voices))
				repaint();
		}, 0, fps, true));
	}

	template<size_t NumVoices>
	void VoiceGrid<NumVoices>::paint(Graphics& g)
	{
		const auto thicc = utils.thicc;
		const auto w = static_cast<float>(getWidth());
		const auto h = static_cast<float>(getHeight());
		const auto y = 0.f;
		const auto w2 = w / static_cast<float>(poly);

		const auto col = getColour(CID::Mod);
		const auto colDarker = col.darker(.2f);
		const auto thicc2 = thicc * 2.f;
		g.setColour(colDarker);

		auto x = 0.f;
		{
			const auto voice = voices[0];
			BoundsF bounds(x, y, w2, h);
			if (voice)
			{
				g.setColour(col);
				g.fillRoundedRectangle(bounds.reduced(thicc2), thicc);
				g.setColour(colDarker);
			}
			x += w2;
		}
		for (auto i = 1; i < poly; ++i)
		{
			const auto voice = voices[i];
			BoundsF bounds(x, y, w2, h);
			g.drawLine(x, y, x, h, thicc);
			if (voice)
			{
				g.setColour(col);
				g.fillRoundedRectangle(bounds.reduced(thicc2), thicc);
				g.setColour(colDarker);
			}
			x += w2;
		}
	}

	template struct VoiceGrid<15>;
}