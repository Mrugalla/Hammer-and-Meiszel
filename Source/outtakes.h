/*

modal material before i changed it from waveform to partials

#pragma once
#include "Comp.h"
#include "../audio/dsp/ModalFilter.h"

namespace gui
{
	struct ModalMaterialView :
		public Comp
	{
		using Material = dsp::modal::Material;

		static constexpr int StepSize = 2;
		static constexpr float StepSizeF = static_cast<float>(StepSize);
		using SampleBuffer = std::vector<float>;

		enum { kWaveformUpdatedCB, kNumCallbacks };

		ModalMaterialView(Utils& u, Material& _material) :
			Comp(u),
			material(_material),
			buffer(),
			drawBipolar(false)
		{
			add(Callback([&]()
			{
				if (material.updated.load())
				{
					update();
					repaint();
				}
			}, kWaveformUpdatedCB, cbFPS::k3_75, true));
		}

		void paint(Graphics& g) override
		{
			if (buffer.empty())
				return;

			const auto h = static_cast<float>(getHeight());
			paintWaveform(g, getColour(CID::Txt), h);
		}

		void resized() override
		{
			buffer.resize(getWidth() / StepSize, 0.f);
			update();
		}

		Material& material;
		SampleBuffer buffer;
		bool drawBipolar;
	private:
		void update()
		{
			updateSampleBuffer();
			material.updated.store(false);
		}

		void updateSampleBuffer()
		{
			auto numSamples = static_cast<double>(material.buffer.size());
			auto samples = material.buffer.data();

			for (auto i = 0; i < buffer.size(); ++i)
			{
				const auto iD = static_cast<double>(i);
				const auto iRatio = iD / static_cast<double>(buffer.size());
				const auto iSample = static_cast<int>(std::round(iRatio * numSamples));
				buffer[i] = static_cast<float>(samples[iSample]);
			}
		}

		void paintWaveform(Graphics& g, Colour c, float h)
		{
			g.setColour(c);
			BoundsF rect(0.f, 0.f, StepSizeF, 0.f);
			if (drawBipolar)
			{
				const auto centreY = h * .5f;
				for (auto i = 0; i < buffer.size(); ++i)
				{
					const auto x = rect.getX();
					const auto smpl = buffer[i];
					auto y = centreY - smpl * centreY;
					auto hgt = centreY - y;
					if (hgt < 0.f)
					{
						hgt = -hgt;
						y = centreY;
					}
					rect.setY(y);
					rect.setHeight(hgt);
					g.fillRect(rect);
					rect.setX(x + StepSizeF);
				}
			}
			else
			{
				for (auto i = 0; i < buffer.size(); ++i)
				{
					const auto x = rect.getX();
					const auto smpl = std::abs(buffer[i]);
					const auto y = h - smpl * h;
					const auto hgt = h - y;
					rect.setY(y);
					rect.setHeight(hgt);
					g.fillRect(rect);
					rect.setX(x + StepSizeF);
				}
			}
		}
	};
}

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
*/