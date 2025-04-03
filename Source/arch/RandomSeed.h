#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <random>

namespace arch
{
	struct RandSeed
	{
		using String = juce::String;
		using Props = juce::PropertiesFile;
		using RandJUCE = juce::Random;

		// user props, id
		RandSeed(Props&, String&&);

		// seedUp
		void updateSeed(bool);

		float operator()();

	private:
		Props& user;
		String id;
		std::random_device rd;
		std::mt19937 mt;
		std::uniform_real_distribution<float> dist;
		int seed;
	};

	using RandFunc = std::function<void(RandSeed&)>;
}