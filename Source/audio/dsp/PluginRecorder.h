#pragma once
#include "../Using.h"

namespace dsp
{
	struct PluginRecorder
	{
		PluginRecorder();

		// sampleRate
		void prepare(double);

		// samples, numChannels, numSamples
		void operator()(double* const*, int, int) noexcept;

		const AudioBufferF& getRecording();

	private:
		AudioBuffer recording;
		AudioBufferF outBuffer;
		int writeHead;

		// samples, numChannels, numSamples
		void normalize(float* const*, int, int) noexcept;
	};
}


