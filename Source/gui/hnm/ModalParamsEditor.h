#pragma once
#include "../../audio/dsp/hnm/modal/Axiom.h"
#include "KnobHnM.h"

namespace gui
{
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
		KnobHnM blend, spreizung, harmonie, kraft, reso, resoDamp, feedback;
		LabelGroup octSemiGroup, knobLabelsGroup;
	};
}