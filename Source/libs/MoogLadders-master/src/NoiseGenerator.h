#pragma once
#include <vector>
#include <stdint.h>
#include <exception>
#include <array>
#include <random>

#include "Util.h"

namespace moog
{

	// +/-0.05dB above 9.2Hz @ 44,100Hz
	class PinkingFilter
	{
		double b0, b1, b2, b3, b4, b5, b6;
	public:
		PinkingFilter() :
			b0(0), b1(0), b2(0), b3(0), b4(0), b5(0), b6(0)
		{}

		double operator()(double s) noexcept
		{
			b0 = .99886 * b0 + s * .0555179;
			b1 = .99332 * b1 + s * .0750759;
			b2 = .96900 * b2 + s * .1538520;
			b3 = .86650 * b3 + s * .3104856;
			b4 = .55000 * b4 + s * .5329522;
			b5 = -.7616 * b5 - s * .0168980;
			auto pink = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + (s * .5362)) * .11;
			b6 = s * .115926;
			return pink;
		}
	};

	class BrowningFilter
	{
		double l;
	public:
		BrowningFilter() :
			l(0)
		{}

		double operator()(const double s) noexcept
		{
			auto brown = (l + (.02 * s)) / 1.02;
			l = brown;
			return brown * 3.5; // compensate for gain
		}
	};

	struct NoiseSource
	{
		NoiseSource() :
			engine(),
			dist(-1, 1)
		{}

		double getWhite() noexcept
		{
			return dist(engine);
		}

		double getPink() noexcept
		{
			return pink(getWhite());
		}

		double getBrown() noexcept
		{
			return brown(getWhite());
		}

	protected:
		std::mt19937 engine;
		std::uniform_real_distribution<double> dist;
		PinkingFilter pink;
		BrowningFilter brown;
	};

	// Note! This noise is only valid for 44100 because of the hard-coded filter coefficients
	struct NoiseGenerator
	{
		enum class NoiseType
		{
			WHITE,
			PINK,
			BROWN
		};

		std::vector<float> produce(NoiseType t, double sampleRate, int numChannels, double seconds)
		{
			const auto samplesToGenerate = static_cast<int>(sampleRate * seconds * numChannels);
			std::vector<float> samples;
			samples.reserve(samplesToGenerate);

			NoiseSource noise;
			if (t == NoiseType::WHITE)
				for (auto s = 0; s < samplesToGenerate; ++s)
					samples[s] = noise.getWhite();
			else if (t == NoiseType::PINK)
				for (auto s = 0; s < samplesToGenerate; ++s)
					samples[s] = noise.getPink();
			else if (t == NoiseType::BROWN)
				for (auto s = 0; s < samplesToGenerate; ++s)
					samples[s] = noise.getBrown();
			
			return samples;
		}
	};
}

