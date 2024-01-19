#pragma once
#include "../Using.h"

namespace dsp
{
	struct PinkNoise
	{
		static constexpr int Size = 1 << 11;

		/* targetDb */
		PinkNoise(double = -24.);

		double rms() noexcept;

		double* data() noexcept;

		const double* data() const noexcept;

	protected:
		std::array<double, Size> noise;

		void synthesizeWhiteNoise() noexcept;

		void pinkenNoise() noexcept;
	};

	template<size_t NumSteps>
	struct AutoGain
	{
		static constexpr double NumStepsD = static_cast<double>(NumSteps);

		AutoGain();

		/* processFunc(smpls, parameterValue, numSamples) */
		void prepareGains(std::function<void(double*, double, int)>&&);

		// valNorm
		void updateParameterValue(double) noexcept;

		// smpl
		double operator()(double) const noexcept;

		double operator()() const noexcept;

	protected:
		std::array<double, NumSteps + 2> gains;
		double gain;
	};

	using AutoGain2 = AutoGain<2>;
	using AutoGain3 = AutoGain<3>;
	using AutoGain5 = AutoGain<5>;
	using AutoGain7 = AutoGain<7>;
}