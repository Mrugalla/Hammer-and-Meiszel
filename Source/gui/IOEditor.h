#pragma once
#include "Knob.h"
#include "VoiceGrid.h"

namespace gui
{
	struct IOEditor :
		public Comp
	{
		enum { kWet, kMix, kOut, kMacro, kSidePanelParams };
		enum { kMacroRel, kMacroSwap, kPower, kDelta, kMidSide, kNumButtons };
		enum { cbMacroRel };

		struct SidePanelParam
		{
			SidePanelParam(Utils&);

			// editor, name, pID
			void init(IOEditor&, const String&, PID);

			void setBounds(BoundsF);

			Layout layout;
			Label label;
			Knob param;
			ModDial modDial;
		};

		IOEditor(Utils&);

		void paint(Graphics&) override;

		void resized();

	protected:
		Label titleXen, titleRefPitch, titleMasterTune, titleMacro;
		Knob xen, refPitch, masterTune, macro;
		ModDial modDialXen, modDialRefPitch, modDialMasterTune;
		std::array<SidePanelParam, kSidePanelParams> sidePanelParams;
		std::array<Button, kNumButtons> buttons;
		VoiceGrid<dsp::AutoMPE::VoicesSize> voiceGrid;
		LabelGroup labelGroup, tuningLabelGroup;

		void initMacroSwap();

		void initMacroRel();

		void initKnobs();

		void initSidePanels();

		void initVoiceGrid();

		void initButtons();
	};
}