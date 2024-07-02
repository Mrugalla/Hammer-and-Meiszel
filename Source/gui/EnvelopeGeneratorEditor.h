#pragma once
#include "Knob.h"
#include "../audio/dsp/EnvelopeGenerator.h"

namespace gui
{
	struct EnvelopeGeneratorEditor :
		public Comp
	{
		using EnvGen = dsp::EnvelopeGenerator;

		EnvelopeGeneratorEditor(Utils& u, const EnvGen& _envGen) :
			Comp(u),
			envGen(_envGen)
		{
		}

		const EnvGen& envGen;
	};
}