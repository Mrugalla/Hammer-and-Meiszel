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

		enum { kTSCheckCB, numCallbacks };

		enum { kAttack, kDecay, kSustain, kRelease, kNumParameters };
		static constexpr int kTitle = kNumParameters;

		struct PIDsTemposync
		{
			PID atk, dcy, rls, temposync;
		};

		struct EnvGenView :
			public Comp
		{
			// utils, sustain
			EnvGenView(Utils&, PID);

			// attack, decay, release, isTemposync
			void init(PID, PID, PID, bool);

			void resized() override;

			void paint(Graphics&) override;

		protected:
			Param& susParam;
			Param *atkParam, *dcyParam, *rlsParam;
			Ruler ruler;
			Path curve, curveMod;
			float atkV, dcyV, susV, rlsV, atkModV, dcyModV, susModV, rlsModV;

			// isTemposync
			void initRuler(bool);

			// path c, atkratio, dcyratio, sus, rlsratio
			void updateCurve(Path&, float,
				float, float, float) noexcept;

			bool updateCurve() noexcept;
		};

		// utils, title, attack, decay, sustain, release, temposyncPIDs
		EnvelopeGeneratorMultiVoiceEditor(Utils&, const String&,
			PID, PID, PID, PID, PIDsTemposync* = nullptr);

		void paint(Graphics&) override;

		void resized() override;

	protected:
		std::array<Label, kNumParameters + 1> labels;
		EnvGenView envGenView;
		std::array<Knob, kNumParameters> knobs;
		std::array<ModDial, kNumParameters> modDials;
		LabelGroup adsrLabelsGroup;
		Button temposync;
		ButtonRandomizer buttonRandomizer;
		bool temposyncEnabled;
	};
}