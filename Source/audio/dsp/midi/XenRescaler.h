#pragma once
#include "MPESplit.h"
#include <functional>

namespace dsp
{
	class XenRescaler
	{
		using NoteOnFunc = std::function<void(const MidiMessage&, MidiBuffer&,
			double, double, double, double, int)>;
		using NoteOnFuncs = std::array<NoteOnFunc, 2>;

	public:
		enum class Type
		{
			Rescale,
			Nearest
		};

		XenRescaler();

		/* midiIn, midiOutAdded, xen, basePitch, masterTune, pitchbendRange, type */
		void operator()(MidiBuffer&, MidiBuffer&,
			double, double, double, double, Type);

	private:
		MidiMessage curNote;
		NoteOnFuncs noteOnFuncs;

		/* midiOut, velo, freq, pitchbendRange, channel, ts */
		void processNoteOn(MidiBuffer&, float,
			double, double, int, int);

		/* midiOut, ts */
		void processNoteOff(MidiBuffer&, int);
	};

	struct XenRescalerMPE
	{
		using Type = XenRescaler::Type;

		XenRescalerMPE(MPESplit&);

		/* midi, xen, basePitch, masterTune, pitchbendRange, numSamples, type */
		void operator()(MidiBuffer&,
			double, double, double,
			double, int, Type);

	private:
		MidiBuffer buffer;
		std::array<XenRescaler, NumMPEChannels> xenRescaler;
		MPESplit& mpe;
	};
}