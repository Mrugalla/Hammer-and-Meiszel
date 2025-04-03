#include "RandomSeed.h"

namespace arch
{
	RandSeed::RandSeed(Props& _props, String&& _id) :
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

	void RandSeed::updateSeed(bool seedUp)
	{
		seed += seedUp ? 1 : -1;
		mt.seed(seed);
		user.setValue(id, seed);
	}

	float RandSeed::operator()()
	{
		return dist(mt);
	}
}