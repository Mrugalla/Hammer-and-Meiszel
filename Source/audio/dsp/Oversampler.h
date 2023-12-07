#pragma once
#include "WHead.h"
#include "Convolver.h"

namespace dsp
{
	struct Oversampler
	{
		static constexpr double LPCutoff = 20000.;
		using OversamplerBuffer = std::array<std::array<double, BlockSize2x>, NumChannels>;
		using IR = ImpulseResponse<double, 1 << 8>;
		using Convolvr = Convolver<double, 1 << 8>;

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
		IR irUp, irDown;
		Convolvr filterUp, filterDown;
	public:
		double sampleRateUp;
		int numSamplesUp;
		bool enabled;
	};
}