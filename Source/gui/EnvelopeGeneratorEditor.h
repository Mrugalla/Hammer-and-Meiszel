#pragma once
#include "Knob.h"
#include "../audio/dsp/EnvelopeGenerator.h"
#include "Ruler.h"
#include "ButtonRandomizer.h"

namespace gui
{
	struct EnvelopeGeneratorMultiVoiceEditor :
		public Comp
	{
		using EnvGen = dsp::EnvelopeGenerator;
		using EnvGenMultiVoice = dsp::EnvGenMultiVoice;

		enum { kAttack, kDecay, kSustain, kRelease, kNumParameters };
		static constexpr int kTitle = kNumParameters;

		struct EnvGenView :
			public Comp
		{
			// utils, attack, decay, sustain, release
			EnvGenView(Utils&, PID, PID, PID, PID);

			void resized() override;

			void paint(Graphics&) override;

		protected:
			Param &atkParam, &dcyParam, &susParam, &rlsParam;
			Ruler ruler;
			Path curve, curveMod;
			float atkV, dcyV, susV, rlsV, atkModV, dcyModV, susModV, rlsModV;

			void initRuler();

			// path c, atkratio, dcyratio, sus, rlsratio
			void updateCurve(Path&, float,
				float, float, float) noexcept;

			bool updateCurve() noexcept;
		};

		// utils, title, attack, decay, sustain, release
		EnvelopeGeneratorMultiVoiceEditor(Utils&, const String&,
			PID, PID, PID, PID);

		void paint(Graphics&) override;

		void resized() override;

	protected:
		std::array<Label, kNumParameters + 1> labels;
		EnvGenView envGenView;
		std::array<Knob, kNumParameters> knobs;
		std::array<ModDial, kNumParameters> modDials;
		LabelGroup adsrLabelsGroup;
		ButtonRandomizer buttonRandomizer;
	};
}