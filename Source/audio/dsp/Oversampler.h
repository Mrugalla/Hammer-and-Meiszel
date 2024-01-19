#pragma once
#include "WHead.h"
#include "Convolver.h"

namespace dsp
{
	struct Oversampler
	{
		static constexpr double LPCutoff = 20000.;
		using OversamplerBuffer = std::array<std::array<double, BlockSize2x>, NumChannels>;

		struct BufferInfo
		{
			double *smplsL, *smplsR;
			int numChannels, numSamples;
		};

		Oversampler();

		/* sampleRate, enabled */
		void prepare(const double, bool);

		/* samples, numChannels, numSamples */
		BufferInfo upsample(double* const*, int, int) noexcept;

		/* samplesOut, numSamples */
		void downsample(double* const*, int) noexcept;

		int getLatency() const noexcept;

	private:
		double sampleRate;
		OversamplerBuffer bufferUp;
		BufferInfo bufferInfo;
		WHead2x wHead;
		ImpulseResponseD8 irUp, irDown;
		ConvolverD8 filterUp, filterDown;
	public:
		double sampleRateUp;
		int numSamplesUp;
		bool enabled;
	};
}