#pragma once
#include "../Button.h"
#include "../../audio/dsp/hnm/modal/Material.h"
#include "../Ruler.h"

namespace gui
{
	struct ModalMaterialEditor :
		public Comp,
		public FileDragAndDropTarget,
		public DragAndDropTarget
	{
		static constexpr double Sensitive = .1;

		using Status = dsp::modal::StatusMat;
		using Material = dsp::modal::Material;
		using PeakInfo = dsp::modal::Partial;
		using Actives = dsp::modal::ActivesArray;
		static constexpr int NumPartials = dsp::modal::NumPartials;
		
		struct Partial
		{
			Partial();
			float x, y;
		};

		using Partials = std::array<Partial, NumPartials>;

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
			std::array<bool, NumPartials> selection;

			void updateSelection(Partials&) noexcept;
		};

		enum
		{
			kMaterialUpdatedCB = 0,
			kStrumCB = 1,
			kNumStrumsCB = kStrumCB + NumPartials,
			kXenUpdatedCB,
			kNumCallbacks
		};

		ModalMaterialEditor(Utils&, Material&, Actives&);

		void initRuler();

		void updateRuler();

		void paint(Graphics&) override;

		// g, h, unselectedPartialCol, idx
		void paintPartial(Graphics&, float, Colour, int);

		void resized() override;

		// soloActive
		void updateActives(bool);

		Material& material;
		Actives& actives;
		Ruler ruler;
		Partials partials;
		DragAnimationComp dragAniComp;
		Draggerfall draggerfall;
		PointF dragXY;
		float freqRatioRange;
	private:
		arch::XenManager::Info xenInfo;

		void updatePartials();

		void updatePartialsRatios();

		void mouseEnter(const Mouse&) override;

		void mouseExit(const Mouse&) override;

		void mouseMove(const Mouse&) override;

		void mouseDown(const Mouse&) override;

		void mouseDrag(const Mouse&) override;

		void mouseDragRatios(PointD, double, double);

		void mouseUp(const Mouse&) override;

		void mouseWheelMove(const Mouse&, const MouseWheel&) override;

		// going up
		void mouseWheelSnap(bool);

		void loadAudioFile(const File&);

		void updateInfoLabel(const String & = "abcabcabc");

		// FileDragAndDropTarget

		void filesDropped(const StringArray&, int, int) override;

		bool isAudioFile(const String&) const;

		void fileDragEnter(const StringArray&, int, int) override;

		void fileDragExit(const StringArray&) override;

		bool isInterestedInFileDrag(const StringArray&) override;

		// DragAndDropTarget

		bool isInterestedInDragSource(const SourceDetails&) override;

		void itemDropped(const SourceDetails&) override;

		void itemDragExit(const SourceDetails&) override;

		const File getTheDnDFile() const;
	};
}