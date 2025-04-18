#pragma once
#include "Knob.h"
#include "VoiceGrid.h"
#include "ButtonPower.h"

namespace gui
{
	struct IOEditor :
		public Comp
	{
		enum { kPoly, kWet, kMix, kOut, kMacro, kSidePanelParams };
		enum
		{
			kMacroRel,
			kMacroSwap,
#if PPDHasDelta
			kDelta,
#endif
			kMidSide,
			kXenSnap,
			kNumButtons
		};
		enum { cbMacroRel };

		struct SidePanelParam
		{
			SidePanelParam(Utils&);

			// editor, name, pID
			void init(IOEditor&, const String&, PID);

			void setVisible(bool);

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
		Label titleXen, titleAnchor, titleMasterTune, titlePitchbend, titleMacro;
		Knob xen, anchor, masterTune, pitchbend, macro;
		ModDial modDialXen, modDialRefPitch, modDialMasterTune, modDialPitchbend;
		std::array<SidePanelParam, kSidePanelParams> sidePanelParams;
		std::array<Button, kNumButtons> buttons;
		ButtonPower buttonPower;
		VoiceGrid<dsp::AutoMPE::VoicesSize> voiceGrid;
		LabelGroup labelGroup, tuningLabelGroup;
		bool keySelectorEnabled;

		void initMacroSwap();

		void initMacroRel();

		void initKnobs();

		void initSidePanels();

		void initVoiceGrid();

		void initButtons();
	};
}