#pragma once
#include "../../Using.h"

namespace dsp
{
	struct MPESplit
	{
		static constexpr int Size = NumMIDIChannels + 1;
		using Buffers = std::array<MidiBuffer, Size>;
		static constexpr int Sysex = 0;

		MPESplit();

		void operator()(MidiBuffer&, int);

		MidiBuffer& operator[](int ch) noexcept;

		const MidiBuffer& operator[](int ch) const noexcept;

	protected:
		Buffers buffers;
	};
}