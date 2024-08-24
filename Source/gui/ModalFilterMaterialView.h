#pragma once
#include "Button.h"
#include "../audio/dsp/modal2/Material.h"

namespace gui
{
	struct ModalMaterialView :
		public Comp,
		public FileDragAndDropTarget
	{
		static constexpr float Sensitive = .1f;

		using Status = dsp::modal2::Status;
		using Material = dsp::modal2::Material;
		using PeakInfo = dsp::modal2::PeakInfo;
		using Actives = dsp::modal2::ActivesArray;
		static constexpr int NumFilters = dsp::modal2::NumFilters;
		
		struct Partial
		{
			Partial();
			float x, y;
		};

		using Partials = std::array<Partial, NumFilters>;

		struct DragAnimationComp :
			public Comp
		{
			DragAnimationComp(Utils&);

			void start();

			void stop();

			void paint(Graphics&) override;

			bool isInterested;
			String error;
		};

		struct Draggerfall
		{
			Draggerfall();

			// partials, w, h
			void resized(Partials&, float, float) noexcept;

			// partials, x, doUpdateSelection
			void updateX(Partials&, float, bool) noexcept;

			// partials, valRel
			void addLength(Partials&, float) noexcept;

			void paint(Graphics& g);

			bool isSelected(int i) const noexcept;

			void clearSelection() noexcept;

			bool selectionEmpty() const noexcept;

			PointF getCoords() const noexcept;

		protected:
			float width, height, xRel, xAbs, lenRel, lenAbs, lenAbsHalf;
			std::array<bool, NumFilters> selection;

			void updateSelection(Partials&) noexcept;
		};

		enum
		{
			kMaterialUpdatedCB = 0,
			kStrumCB = 1,
			kNumStrumsCB = kStrumCB + NumFilters,
			kNumCallbacks
		};

		ModalMaterialView(Utils&, Material&, Actives&);

		void paint(Graphics&) override;

		// g, h, unselectedPartialCol, idx
		void paintPartial(Graphics&, float, Colour, int);

		void resized() override;

		// soloActive
		void updateActives(bool);

		Material& material;
		Actives& actives;
		Partials partials;
		DragAnimationComp dragAniComp;
		Draggerfall draggerfall;
		PointF dragXY;
		float freqRatioRange;
	private:

		void updatePartials();

		void mouseEnter(const Mouse&) override;

		void mouseExit(const Mouse&) override;

		void mouseMove(const Mouse&) override;

		void mouseDown(const Mouse&) override;

		void mouseDrag(const Mouse&) override;

		void mouseUp(const Mouse&) override;

		void mouseWheelMove(const Mouse&, const MouseWheel&) override;

		void loadAudioFile(const File&);

		void filesDropped(const StringArray&, int, int) override;

		bool isAudioFile(const String&) const;

		void fileDragEnter(const StringArray&, int, int) override;

		void fileDragExit(const StringArray&) override;

		bool isInterestedInFileDrag(const StringArray&) override;

		void updateInfoLabel(const String& = "abcabcabc");

		// buttonsHeight
		void updateButtonsPosition(float);
	};
}