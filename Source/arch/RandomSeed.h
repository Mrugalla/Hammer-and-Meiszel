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
		RandSeed(Props& _props, String&& _id) :
			user(_props),
			id(_id.removeCharacters(" ").toLowerCase()),
			mt(rd()),
			dist(0.f, 1.f),
			seed(user.getIntValue(id, 0))
		{
			if (seed != 0)
				return;
			RandJUCE rand;
			seed = rand.nextInt();
			user.setValue(id, seed);
		}

		// seedUp
		void updateSeed(bool seedUp)
		{
			seed += seedUp ? 1 : -1;
			mt.seed(seed);
			user.setValue(id, seed);
		}

		float operator()()
		{
			return dist(mt);
		}

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