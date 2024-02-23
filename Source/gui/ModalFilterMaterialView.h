#pragma once
#include "Label.h"
#include "../audio/dsp/modal/Material.h"

namespace gui
{
	struct ModalMaterialView :
		public Comp
	{
		static constexpr float KnotHitBoxSens = 6.f;

		using Material = dsp::modal::Material;
		using UpdateState = Material::UpdateState;
		using PeakInfo = Material::PeakInfo;
		static constexpr int NumFilters = dsp::modal::NumFilters;

		struct Partial
		{
			Partial() :
				x(0.f), y(0.f)
			{
			}

			float x, y;
		};

		enum
		{
			kMaterialUpdatedCB = 0,
			kStrumCB = 1,
			kNumStrumsCB = kStrumCB + NumFilters,
			kNumCallbacks
		};

		ModalMaterialView(Utils& u, Material& _material) :
			Comp(u),
			infoLabel(u, true),
			material(_material),
			partials(),
			dragXY(),
			selected(-1)
		{
			layout.init
			(
				{ 1 },
				{ 1, 5 }
			);

			addAndMakeVisible(infoLabel);
			makeTextLabel(infoLabel, "", font::nel(), Just::topRight, CID::Hover);

			add(Callback([&]()
			{
				if (material.updateState.load() == UpdateState::UpdateGUI)
				{
					update();
					repaint();
				}
			}, kMaterialUpdatedCB, cbFPS::k30, true));

			const auto fps = cbFPS::k30;
			const auto speed = msToInc(AniLengthMs, fps);

			for(auto i = 0; i < NumFilters; ++i)
				add(Callback([&, speed, i]()
				{
					auto& phase = callbacks[kStrumCB + i].phase;
					phase -= speed;
					if (phase <= 0.f)
					{
						phase = 0.f;
						callbacks[kStrumCB + i].active = false;
					}
					repaint();
				}, kStrumCB + i, fps, false));
		}

		void paint(Graphics& g) override
		{
			auto const h = static_cast<float>(getHeight());

			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto strumPhase = callbacks[kStrumCB + i].phase;
				const auto knotW = utils.thicc
					* (selected == i ? 2.f : 1.f)
					+ utils.thicc * strumPhase * 2.f;
				const auto knotWHalf = knotW * .5f;

				const auto x = partials[i].x;
				const auto y = partials[i].y;

				if(selected == i)
					g.setColour(Colours::c(CID::Interact));
				else
					g.setColour(Colours::c(CID::Txt));
				g.fillRect(x - knotWHalf, y, knotW, h);

				g.setColour(Colours::c(CID::Interact));
				g.fillRect(x - knotWHalf, y - knotWHalf, knotW, knotW);
			}
		}

		void resized() override
		{
			layout.resized(getLocalBounds().toFloat());
			layout.place(infoLabel, 0, 0, 1, 1);
			update();
		}

		Label infoLabel;
		Material& material;
		std::array<Partial, NumFilters> partials;
		PointF dragXY;
		int selected;
	private:
		void update()
		{
			auto const w = static_cast<float>(getWidth());
			auto const h = static_cast<float>(getHeight());

			auto maxRatio = 0.f;
			for (auto& peakInfo : material.peakInfos)
				maxRatio = std::max(maxRatio, static_cast<float>(peakInfo.ratio));
			maxRatio -= 1.f;
			const auto maxRatioInv = 1.f / maxRatio;

			for (auto i = 0; i < NumFilters; ++i)
			{
				auto& peakInfo = material.peakInfos[i];

				auto const x = ((static_cast<float>(peakInfo.ratio) - 1.f)) * maxRatioInv * w;
				auto const y = h - static_cast<float>(peakInfo.mag) * h;

				partials[i].x = x;
				partials[i].y = y;
			}

			material.updateState.store(UpdateState::Idle);
		}

		void mouseExit(const Mouse&) override
		{
			selected = -1;
			infoLabel.setText("");
			repaint();
		}

		void mouseMove(const Mouse& mouse) override
		{
			const auto hitboxDist = utils.thicc * KnotHitBoxSens;

			auto minDist = static_cast<float>(getWidth());
			auto minDistIdx = -1;
			for (auto i = 0; i < NumFilters; ++i)
			{
				auto const x = static_cast<float>(partials[i].x);
				const auto xDist = std::abs(mouse.x - x);
				if (xDist < hitboxDist && xDist < minDist)
				{
					minDist = xDist;
					minDistIdx = i;
				}
			}

			if (minDistIdx == -1)
			{
				selected = -1;
				infoLabel.setText("");
				return repaint();
			}

			if (selected != minDistIdx)
			{
				selected = minDistIdx;
				callbacks[kStrumCB + selected].start(1.f);
				updateInfoLabel();
			}
		}

		void mouseDown(const Mouse& mouse) override
		{
			if (selected == -1)
				return;

			dragXY = { static_cast<float>(mouse.x), static_cast<float>(mouse.y) };
		}

		void mouseDrag(const Mouse& mouse) override
		{
			if (selected == -1)
				return;
			
			hideCursor();

			const auto minDimen = static_cast<float>(std::min(getWidth(), getHeight()));
			const auto speed = 3.f * utils.thicc / minDimen;
			const auto dragDist = ((mouse.position - dragXY) * speed).toDouble();

			if(material.updateState.load() == UpdateState::Idle)
			{
				auto& peakInfo = material.peakInfos[selected];
				peakInfo.mag = juce::jlimit(0., 100., peakInfo.mag - dragDist.y);
				peakInfo.ratio = juce::jlimit(1., 100., peakInfo.ratio + dragDist.x);
				material.updatePeakInfosFromGUI();
				updateInfoLabel();
			}

			dragXY = mouse.position;
		}

		void mouseUp(const Mouse& mouse) override
		{
			if (selected == -1)
				return;

			mouseDrag(mouse);

			PointF pos
			(
					static_cast<float>(partials[selected].x),
					static_cast<float>(partials[selected].y)
			);

			PointF screenPos = localPointToGlobal(pos);

			showCursor(screenPos);
		}

		void updateInfoLabel()
		{
			const auto mag = material.peakInfos[selected].mag;
			const auto rat = material.peakInfos[selected].ratio;
			infoLabel.setText("mag: " + String(mag, 2) + ", rat : " + String(rat, 2));
		}
	};
}