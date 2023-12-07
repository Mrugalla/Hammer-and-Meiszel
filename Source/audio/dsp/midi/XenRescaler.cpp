#include "XenRescaler.h"
#include "../../../arch/Math.h"

namespace dsp
{
	XenRescaler::XenRescaler() :
		curNote(MidiMessage::noteOn(1, 0, Uint8(0))),
		noteOnFuncs()
	{
		noteOnFuncs[static_cast<int>(Type::Rescale)] = [&](const MidiMessage& msg, MidiBuffer& buffer,
			double xen, double basePitch, double masterTune,
			double pitchbendRange, int ts)
			{
				const auto channel = msg.getChannel();
				const auto noteNumber = static_cast<double>(msg.getNoteNumber());
				const auto freq = math::noteToFreqHz(noteNumber, xen, basePitch, masterTune);
				processNoteOn(buffer, msg.getFloatVelocity(), freq, pitchbendRange, channel, ts);
			};

		noteOnFuncs[static_cast<int>(Type::Nearest)] = [&](const MidiMessage& msg, MidiBuffer& buffer,
			double xen, double basePitch, double masterTune,
			double pitchbendRange, int ts)
			{
				const auto channel = msg.getChannel();
				const auto noteNumber = static_cast<double>(msg.getNoteNumber());
				const auto freq = math::noteToFreqHz(noteNumber);
				const auto cFreq = math::closestFreq(freq, xen, basePitch, masterTune);
				processNoteOn(buffer, msg.getFloatVelocity(), cFreq, pitchbendRange, channel, ts);
			};
	}

	void XenRescaler::operator()(MidiBuffer& midi, MidiBuffer& buffer,
		double xen, double basePitch, double masterTune,
		double pitchbendRange, Type type)
	{
		for (const auto it : midi)
		{
			const auto ts = it.samplePosition;
			const auto msg = it.getMessage();
			if (msg.isNoteOn())
			{
				auto& processNoteOn = noteOnFuncs[static_cast<int>(type)];
				processNoteOn(msg, buffer, xen, basePitch, masterTune, pitchbendRange, ts);
			}
			else if (msg.isNoteOff())
				processNoteOff(buffer, ts);
			else
				buffer.addEvent(msg, ts);
		}
	}

	void XenRescaler::processNoteOn(MidiBuffer& buffer, float velocity,
		double freq, double pitchbendRange, int channel, int ts)
	{
		curNote.setChannel(channel);
		curNote.setVelocity(velocity);

		const auto note = math::freqHzToNote(freq);
		const auto noteRound = std::round(note);
		curNote.setNoteNumber(static_cast<int>(noteRound));

		const auto noteFrac = (note - noteRound) / pitchbendRange;
		const auto pitchbend = noteFrac * PitchbendRangeHalf + PitchbendRangeHalf;

		buffer.addEvent(MidiMessage::pitchWheel(channel, static_cast<int>(pitchbend)), ts);
		buffer.addEvent(curNote, ts);
	}

	void XenRescaler::processNoteOff(MidiBuffer& buffer, int ts)
	{
		const auto channel = curNote.getChannel();
		const auto note = curNote.getNoteNumber();
		buffer.addEvent(MidiMessage::noteOff(channel, note), ts);
	}

	XenRescalerMPE::XenRescalerMPE(MPESplit& _mpe) :
		buffer(),
		xenRescaler(),
		mpe(_mpe)
	{
	}

	void XenRescalerMPE::operator()(MidiBuffer& midiMessages,
		double xen, double basePitch, double masterTune,
		double pitchbendRange, int numSamples, Type type)
	{
		buffer.clear();

		for (auto ch = 0; ch < NumMPEChannels; ++ch)
		{
			auto& rescaler = xenRescaler[ch];
			auto& midi = mpe[ch + 2];
			
			rescaler(midi, buffer, xen, basePitch, masterTune, pitchbendRange, type);
		}

		midiMessages.addEvents(buffer, 0, numSamples, 0);
	}
}