#pragma once
#include "../Using.h"

namespace dsp
{
	struct PluginRecorder
	{
		PluginRecorder() :
			recording(),
			outBuffer(),
			writeHead(0)
		{}

		void prepare(double sampleRate)
		{
			const auto numSamples = static_cast<int>(sampleRate * 10.);
			recording.setSize(2, numSamples, false, true, false);
			outBuffer.setSize(2, numSamples, false, true, false);
		}

		void operator()(double* const* samples, int numChannels, int numSamples) noexcept
		{
			auto recSamples = recording.getArrayOfWritePointers();
			const auto recNumSamples = recording.getNumSamples();
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto smpls = samples[ch];
				auto recSmpls = recSamples[ch];
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto wH = (writeHead + s) % recNumSamples;
					recSmpls[wH] = smpls[s];
				}
			}
			writeHead = (writeHead + numSamples) % recNumSamples;
		}

		const AudioBufferF& getRecording()
		{
			const auto numChannels = 2;
			const auto numSamples = recording.getNumSamples();
			auto rHead = (writeHead + 1) % numSamples;
			auto recBuf = recording.getArrayOfReadPointers();
			auto outBuf = outBuffer.getArrayOfWritePointers();
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto rec = recBuf[ch];
				auto buf = outBuf[ch];
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto rH = (rHead + s) % numSamples;
					buf[s] = static_cast<float>(rec[rH]);
				}
			}
			normalize(outBuf, numChannels, numSamples);
			return outBuffer;
		}

	private:
		AudioBuffer recording;
		AudioBufferF outBuffer;
		int writeHead;

		void normalize(float* const* samples, int numChannels, int numSamples)
		{
			auto mag = 0.f;
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto smpls = samples[ch];
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto absSmpl = std::abs(smpls[s]);
					mag = std::max(mag, absSmpl);
				}
			}
			if (mag == 0.f || mag == 1.f)
				return;
			auto g = 1.f / mag;
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::multiply(samples[ch], g, numSamples);
		}
	};
}