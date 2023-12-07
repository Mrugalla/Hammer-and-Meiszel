#pragma once
#include "../../Using.h"

namespace dsp
{
	struct SampleDelay
	{
		SampleDelay();

		/* midi, numSamples */
		void operator()(MidiBuffer&, int);

		MidiBuffer buffer;
		MidiMessage temp;
		bool needTemp;
	};

	class Delay
	{
		struct Msg
		{
			Msg();

			MidiMessage msg;
			int ts;
		};

		static constexpr int Size = 1 << 5;
		static constexpr int Max = Size - 1;
		using Stack = std::array<Msg, Size>;
	public:
		Delay();

		/* midi, numSamples, delayLength*/
		void operator()(MidiBuffer&, int, int);

	private:
		MidiBuffer buffer;
		Stack stack;
		int idx;

		/* midi, delayLength */
		void seperateNotesFromRest(MidiBuffer&, int);

		/* msg, nTs */
		void addToStack(const MidiMessage&, int) noexcept;

		/* midi, numSamples */
		void processDelayedNotes(MidiBuffer&, int);
	};
}