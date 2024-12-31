#pragma once
#include "../../audio/dsp/hnm/modal/Axiom.h"
#include "KnobHnM.h"

namespace gui
{
	struct ModalMainParams :
		public Comp
	{
		using kParam = dsp::modal::kParam;

		//utils, smartKeytrack, blend, spread, harmonie, kraft, reso, resoDamp
		ModalMainParams(Utils&, PID, PID, PID, PID, PID, PID, PID);

		void resized() override;

	protected:
		std::array<Knob, dsp::modal::kNumParams> knobs;
		std::array<ModDial, dsp::modal::kNumParams> modDials;
	};

	struct ModalParamsEditor :
		public Comp
	{
		using kParam = dsp::modal::kParam;

		struct OctSemiSlider :
			public Comp
		{
			//utils, pID, name
			OctSemiSlider(Utils&, PID, String&&);

			void resized() override;

			Label label;
			Knob knob;
			ModDial modDial;
		};

		ModalParamsEditor(Utils&);

		void resized() override;

	protected:
		OctSemiSlider octModal, semiModal, octComb, semiComb, unisonComb;
		KnobHnM smartKeytrack, blend, spreizung, harmonie, kraft, reso, resoDamp, feedback;
		LabelGroup octSemiGroup;
	};
}