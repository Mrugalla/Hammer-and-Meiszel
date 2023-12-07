#include "XenDemoSynth.h"
#include "../../../arch/Math.h"

namespace dsp
{

	XenDemoSynth::Synth::Synth() :
		phase(0.),
		inc(0.),
		env(0.),
		rise(0.),
		fall(0.),
		sampleRateInvTau(1.),
		noteOn(false)
	{}

	void XenDemoSynth::Synth::prepare(double sampleRate) noexcept
	{
		sampleRateInvTau = Tau / sampleRate;
		rise = 500. / sampleRate;
		fall = 80. / sampleRate;
	}

	void XenDemoSynth::Synth::setFreqHz(double freq) noexcept
	{
		inc = freq * sampleRateInvTau;
	}

	double XenDemoSynth::Synth::process() noexcept
	{
		phase += inc;
		if (phase > Pi)
			phase -= Tau;
		const auto sine = std::sin(phase);
		const auto saturated = std::tanh(4. * sine) * Gain;

		if (noteOn)
			env += rise * (1. - env);
		else
			env += fall * (0. - env);

		const auto y = saturated * env;
		return y;
	}

	XenDemoSynth::XenDemoSynth() :
		synth()
	{
	}

	void XenDemoSynth::prepare(double sampleRate)
	{
		synth.prepare(sampleRate);
	}

	void XenDemoSynth::synthesizeRescale(float* smpls, const MidiBuffer& midiIn,
		double xen, double basePitch, double masterTune,
		int numSamples)
	{
		auto s = 0;

		for (const auto it : midiIn)
		{
			const auto ts = it.samplePosition;
			const auto msg = it.getMessage();
			if (msg.isNoteOnOrOff())
			{
				while (s < ts)
				{
					smpls[s] += static_cast<float>(synth.process());
					++s;
				}

				if (msg.isNoteOn())
				{
					const auto noteNumber = static_cast<double>(msg.getNoteNumber());
					const auto freq = math::noteToFreqHz(noteNumber, xen, basePitch, masterTune);
					synth.setFreqHz(freq);
					synth.noteOn = true;
				}
				else
					synth.noteOn = false;
			}
		}

		while (s < numSamples)
		{
			smpls[s] += static_cast<float>(synth.process());
			++s;
		}
	}

	void XenDemoSynth::synthesizeNearest(float* smpls, const MidiBuffer& midiIn,
		double xen, double basePitch,
		double masterTune, int numSamples)
	{
		auto s = 0;

		for (const auto it : midiIn)
		{
			const auto ts = it.samplePosition;
			const auto msg = it.getMessage();
			if (msg.isNoteOnOrOff())
			{
				while (s < ts)
				{
					smpls[s] += static_cast<float>(synth.process());
					++s;
				}

				if (msg.isNoteOn())
				{
					const auto noteNumber = static_cast<double>(msg.getNoteNumber());
					const auto freq = math::noteToFreqHz(noteNumber);
					const auto cFreq = math::closestFreq(freq, xen, basePitch, masterTune);
					synth.setFreqHz(cFreq);
					synth.noteOn = true;
				}
				else
					synth.noteOn = false;
			}
		}

		while (s < numSamples)
		{
			smpls[s] += static_cast<float>(synth.process());
			++s;
		}
	}


	XenSynthMPE::XenSynthMPE(MPESplit& _mpe) :
		voices(),
		mpe(_mpe)
	{}

	void XenSynthMPE::prepare(double sampleRate)
	{
		for (auto& voice : voices)
			voice.prepare(sampleRate);
	}

	void XenSynthMPE::operator()(float* const* samples,
		double xen, double basePitch, double masterTune,
		int numChannels, int numSamples,
		bool playModeVal) noexcept
	{
		if (playModeVal)
			for (auto ch = 0; ch < NumMPEChannels; ++ch)
			{
				auto& voice = voices[ch];
				const auto& midi = mpe[ch + 2];

				voice.synthesizeNearest(samples[0], midi, xen, basePitch, masterTune, numSamples);
			}
		else
			for (auto ch = 0; ch < NumMPEChannels; ++ch)
			{
				auto& voice = voices[ch];
				const auto& midi = mpe[ch + 2];

				voice.synthesizeRescale(samples[0], midi, xen, basePitch, masterTune, numSamples);
			}

		if (numChannels == 2)
			juce::FloatVectorOperations::copy(samples[1], samples[0], numSamples);
	}
}