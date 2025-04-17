#pragma once
#include "Knob.h"
#include "../audio/dsp/Randomizer.h"
#include "ButtonRandomizer.h"

namespace gui
{
	struct RandomizerEditor :
		public Comp
	{
		using RandMod = dsp::Randomizer;

		struct Visualizer :
			public Comp
		{
			Visualizer(Utils&, const RandMod&);

			void resized() override;

			void paint(Graphics&);
		private:
			Image img;
			float y0;
		};

		// randMod, u, rateSync, smooth, complex, dropout
		RandomizerEditor(const RandMod&, Utils&,
			PID, PID, PID, PID);

		void paint(Graphics&);

		void resized() override;
	private:
		Visualizer visualizer;
		Label title, rateSyncLabel, smoothLabel, complexLabel, dropoutLabel;
		Knob rateSync, smooth, complex, dropout;
		ModDial rateSyncMod, smoothMod, complexMod, dropoutMod;
		ButtonRandomizer randomizer;
		LabelGroup labelGroup;
	};
}