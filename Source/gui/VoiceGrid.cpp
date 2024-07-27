#include "VoiceGrid.h"

namespace gui
{
	template<size_t NumVoices>
	VoiceGrid<NumVoices>::VoiceGrid(Utils& u) :
		Comp(u),
		voices()
	{
	}
	
	template<size_t NumVoices>
	void VoiceGrid<NumVoices>::init(const UpdateFunc& updateFunc)
	{
		auto fps = cbFPS::k30;
		//const auto speed = msToInc(AniLengthMs, fps);
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
		const auto w2 = w * NumVoicesInv;

		auto x = 0.f;
		{
			const auto voice = voices[0];
			BoundsF bounds(x, y, w2, h);
			if (voice)
			{
				setCol(g, CID::Mod);
				g.fillRoundedRectangle(bounds, thicc);
			}
			x += w2;
		}
		for (auto i = 1; i < NumVoices; ++i)
		{
			const auto voice = voices[i];
			BoundsF bounds(x, y, w2, h);
			if (voice)
			{
				setCol(g, CID::Mod);
				g.fillRoundedRectangle(bounds, thicc);
			}
			else
			{
				g.setColour(getColour(CID::Mod).darker(.2f));
				g.drawLine(x, y, x, h, thicc);
			}
			x += w2;
		}
	}
}