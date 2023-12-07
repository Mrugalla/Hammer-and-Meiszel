#pragma once
#include "PRM.h"

namespace dsp
{
	template<double SmoothLengthMs, double MinDb>
	struct Gain
	{
		/* defaultValueDb */
		Gain(double);

		/* sampleRate */
		void prepare(double) noexcept;

		/* samples, gainDb, numChannels, numSamples */
		void operator()(double* const*, double, int, int) noexcept;

		/* samples, numChannels, numSamples */
		void applyInverse(double* const*, int, int) noexcept;

		/* smpls, numSamples */
		void applyInverse(double*, int) noexcept;

	private:
		PRMD gainPRM;
		PRMInfoD gainInfo;
	};
}