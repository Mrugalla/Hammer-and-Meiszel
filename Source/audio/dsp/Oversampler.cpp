#include "Oversampler.h"

namespace dsp
{
	// samplesUp, samplesIn, numChannels, numSamples1x
	void zeroStuff(double* const* samplesUp, const double* const* samplesIn,
		int numChannels, int numSamples1x) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto upBuf = samplesUp[ch];
			const auto inBuf = samplesIn[ch];

			for (auto s = 0; s < numSamples1x; ++s)
			{
				const auto s2 = s * 2;
				upBuf[s2] = inBuf[s];
				upBuf[s2 + 1] = 0.;
			}
		}
	}

	// samplesOut, samplesUp, numChannels, numSamples1x
	void decimate(double* const* samplesOut, const double* const* samplesUp,
		int numChannels, int numSamples1x) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto outBuf = samplesOut[ch];
			const auto upBuf = samplesUp[ch];

			for (auto s = 0; s < numSamples1x; ++s)
				outBuf[s] = upBuf[s * 2];
		}
	}

	Oversampler::Oversampler() :
		sampleRate(0.),
		bufferUp(),
		bufferInfo(),
		irUp(), irDown(),
		wHead(),
		filterUp(irUp), filterDown(irDown),
		sampleRateUp(0.),
		numSamplesUp(0),
		enabled(false)
	{
	}

	void Oversampler::prepare(const double _sampleRate, bool _enabled)
	{
		sampleRate = _sampleRate;
		enabled = _enabled;

		if (enabled)
		{
			sampleRateUp = sampleRate * 2.;

			irUp.makeLowpass(sampleRateUp, LPCutoff, true);
			for (auto i = 0; i < irUp.size; ++i)
				irUp[i] *= 2.;
			irDown.makeLowpass(sampleRateUp, LPCutoff, true);
			const auto irSize = static_cast<int>(irUp.size);
			wHead.prepare(irSize);
		}
		else
		{
			sampleRateUp = sampleRate;
		}
	}

	Oversampler::BufferInfo Oversampler::upsample(double* const* samples,
		int numChannels, int numSamples) noexcept
	{
		bufferInfo.numChannels = numChannels;

		if (enabled)
		{
			bufferInfo.numSamples = numSamplesUp = numSamples * 2;
			bufferInfo.smplsL = bufferUp[0].data();
			bufferInfo.smplsR = bufferUp[1].data();
			double* samplesUp[] = { bufferInfo.smplsL, bufferInfo.smplsR };

			wHead(numSamplesUp);
			const auto wHeadData = wHead.data();

			zeroStuff(samplesUp, samples, numChannels, numSamples);
			filterUp.processBlock(samplesUp, wHeadData, numChannels, numSamplesUp);
		}
		else
		{
			bufferInfo.numSamples = numSamples;
			bufferInfo.smplsL = samples[0];
			bufferInfo.smplsR = samples[1];
		}

		return bufferInfo;
	}

	void Oversampler::downsample(double* const* samplesOut, int numSamples) noexcept
	{
		if (enabled)
		{
			const auto numChannels = bufferInfo.numChannels;
			const auto wHeadData = wHead.data();

			double* samplesUp[] = { bufferInfo.smplsL, bufferInfo.smplsR };

			// filter 2x + decimating
			filterDown.processBlock(samplesUp, wHeadData, numChannels, numSamplesUp);
			decimate(samplesOut, samplesUp, numChannels, numSamples);
		}
	}

	int Oversampler::getLatency() const noexcept
	{
		return enabled ? (irUp.getLatency() + irDown.getLatency() / 2) : 0;
	}
}