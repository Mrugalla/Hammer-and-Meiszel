#include "AutoGain.h"
#include "../../arch/Math.h"
#include <random>

namespace dsp
{
	using MersenneTwister = std::mt19937;
	using RandDistribution = std::uniform_real_distribution<double>;

	// PinkNoise

	PinkNoise::PinkNoise(double targetDb) :
		noise()
	{
		synthesizeWhiteNoise();
		pinkenNoise();

		const auto targetRms = math::dbToAmp(targetDb);
		const auto gain = targetRms / rms();

		for (auto& n : noise)
			n *= gain;
	}

	double PinkNoise::rms() noexcept
	{
		return math::getRMS(noise.data(), Size);
	}

	double* PinkNoise::data() noexcept
	{
		return noise.data();
	}

	const double* PinkNoise::data() const noexcept
	{
		return noise.data();
	}

	void PinkNoise::synthesizeWhiteNoise() noexcept
	{
		{ // PROCEDURAL WHITE NOISE
			MersenneTwister mt(420);
			RandDistribution dist(-1., 1.);

			for (auto n = 0; n < Size; ++n)
				noise[n] = dist(mt);
		}
	}

	void PinkNoise::pinkenNoise() noexcept
	{
		{ // WHITE >> PINK NOISE
			std::array<double, 7> b;
			for (auto& a : b)
				a = 0.;
			for (auto n = 0; n < Size; ++n)
			{
				const auto white = noise[n];
				b[0] = 0.99886 * b[0] + white * 0.0555179;
				b[1] = 0.99332 * b[1] + white * 0.0750759;
				b[2] = 0.96900 * b[2] + white * 0.1538520;
				b[3] = 0.86650 * b[3] + white * 0.3104856;
				b[4] = 0.55000 * b[4] + white * 0.5329522;
				b[5] = -0.7616 * b[5] - white * 0.0168980;
				auto pink = white * 0.5362;
				for (auto a : b)
					pink += a;
				b[6] = white * 0.115926;
				noise[n] = pink;
			}
		}
	}

	// AutoGain

	template<size_t NumGains>
	AutoGain<NumGains>::AutoGain() :
		gains(),
		gain{ 1., 1. }
	{
		for (auto& g : gains)
			g = 1.;
	}

	template<size_t NumGains>
	void AutoGain<NumGains>::prepareGains(std::function<void(double*, double, int)>&& processFunc)
	{
		auto pinkNoise = new PinkNoise();
		auto& noise = *pinkNoise;
		auto rms = noise.rms();
		std::vector<double> samples;
		samples.resize(noise.Size);
		for (auto i = 0; i < gains.size(); ++i)
		{
			const auto iD = static_cast<double>(i);
			auto x = iD / NumStepsD;
			x = x > 1. ? 1. : x;
			SIMD::copy(samples.data(), noise.data(), noise.Size);
			processFunc(samples.data(), x, noise.Size);
			const auto nRMS = math::getRMS(samples.data(), noise.Size);
			gains[i] = (nRMS == 0.f || std::isnan(nRMS) || std::isinf(nRMS)) ? 0.f : rms / nRMS;
		}
		delete pinkNoise;
	}

	template<size_t NumGains>
	void AutoGain<NumGains>::updateParameterValue(double valNorm, int ch) noexcept
	{
		const auto x = valNorm * NumStepsD;
		const auto xFloor = std::floor(x);
		const auto iF = static_cast<int>(xFloor);
		const auto iC = iF + 1;
		const auto xFrac = x - xFloor;
		const auto gF = gains[iF];
		const auto gC = gains[iC];
		const auto gRange = gC - gF;
		gain[ch] = gF + xFrac * gRange;
	}

	template<size_t NumGains>
	double AutoGain<NumGains>::operator()(double smpl, int ch) const noexcept
	{
		return smpl * gain[ch];
	}

	template<size_t NumGains>
	double AutoGain<NumGains>::operator()(int ch) const noexcept
	{
		return gain[ch];
	}

	template struct AutoGain<2>;
	template struct AutoGain<3>;
	template struct AutoGain<5>;
	template struct AutoGain<7>;
}