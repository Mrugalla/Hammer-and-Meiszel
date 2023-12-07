#pragma once

namespace dsp
{
	template<typename Float>
	struct PhaseInfo
	{
		/* phase, retrig */
		PhaseInfo(Float = static_cast<Float>(0), bool = false);

		Float phase;
		bool retrig;
	};

	using PhaseInfoF = PhaseInfo<float>;
	using PhaseInfoD = PhaseInfo<double>;

	template<typename Float>
	struct Phasor
	{
		using Phase = PhaseInfo<Float>;

		void setFrequencyHz(Float) noexcept;

		/* phase, inc */
		Phasor(Float = static_cast<Float>(0), Float = static_cast<Float>(0));

		/* fsInv */
		void prepare(Float) noexcept;

		void reset(Float = static_cast<Float>(0)) noexcept;

		Phase operator()() noexcept;

		/* numSamples */
		Phase operator()(int) noexcept;

		Phase phase;
		Float inc, fsInv;
	};

	using PhasorF = Phasor<float>;
	using PhasorD = Phasor<double>;
}