#include "NoiseSynth.h"
#include <juce_dsp/juce_dsp.h>

namespace dsp
{
	NoiseSynth::NoiseSynth() :
		rand(),
		noise(),
		rHead(0),
		flipIdx(0)
	{
		juce::dsp::FirstOrderTPTFilter<double> lp;
		lp.snapToZero();
		lp.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
		lp.setCutoffFrequency(20000.);
		juce::dsp::ProcessSpec spec;
		spec.sampleRate = 44100.;
		spec.maximumBlockSize = Size;
		spec.numChannels = 1;
		lp.prepare(spec);

		const auto min12db = math::dbToAmp(-12.);
		for (auto i = 0; i < Size; ++i)
		{
			const auto whiteNoise = (rand.nextDouble() * 2. - 1.) * min12db;
			const auto pinkNoise = lp.processSample(0, whiteNoise);
			noise[i] = pinkNoise;
		}
	}

	void NoiseSynth::operator()(double** samples, double blend,
		int numChannels, int numSamples) noexcept
	{
		if (blend == 0.)
			return;
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dry = smpls[s];
				const auto wet = noise[rHead];

				smpls[s] = dry + blend * (wet - dry);
				rHead = (rHead + 1) & Max;

				++flipIdx;
				if (flipIdx == FlipLen)
				{
					flipIdx = 0;
					rHead = rand.nextInt(Size);
				}
			}
		}
	}
}