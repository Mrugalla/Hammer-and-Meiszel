#pragma once
#include "MPESplit.h"

namespace dsp
{
	class XenDemoSynth
	{
		struct Synth
		{
			static constexpr double Gain = .2;

			Synth();

			void prepare(double) noexcept;

			void setFreqHz(double) noexcept;

			double process() noexcept;

		private:
			double phase, inc, env, rise, fall, sampleRateInvTau;
		public:
			bool noteOn;
		};

	public:
		XenDemoSynth();

		void prepare(double);

		/* smpls, midiIn, xen, basePitch, masterTune, numSamples */
		void synthesizeRescale(float*, const MidiBuffer&,
			double, double, double, int);

		/* smpls, midiIn, xen, basePitch, masterTune, numSamples */
		void synthesizeNearest(float*, const MidiBuffer&,
			double, double, double, int);

	private:
		Synth synth;
	};

	struct XenSynthMPE
	{
		XenSynthMPE(MPESplit&);

		void prepare(double);

		/* samples, xen, basePitch, masterTune, numChannels, numSamples, playModeVal */
		void operator()(float* const*, double, double, double,
			int, int, bool) noexcept;

	private:
		std::array<XenDemoSynth, NumMPEChannels> voices;
		MPESplit& mpe;
	};
}