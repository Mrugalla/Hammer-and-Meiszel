#pragma once
#include "../Using.h"

namespace dsp
{
	// sample, ceiling
	template<typename Float>
	Float hardclip(Float, Float) noexcept;

	// sample, threshold, ratio
	template<typename Float>
	Float ratioclip(Float, Float, Float) noexcept;

	// sample, ceiling
	template<typename Float>
	Float softclipCubic(Float, Float) noexcept;
	
	// sample, ceiling, alpha[1,10]
	template<typename Float>
	Float softclipAtan(Float, Float, Float) noexcept;

	// sample, ceiling, k[1,10]
	// smooth transition between soft and hard but
	// low k values don't approximate to ceiling well
	template<typename Float>
	Float softclipSigmoid(Float, Float, Float) noexcept;

	// sample, ceiling, k[1,10]
	//this one clips the ceiling at all times, but less on higher k
	template<typename Float>
	Float softclipAtan2(Float, Float, Float) noexcept;
	
	// sample, ceiling, k[1,10]
	//same as Atan2, but smoother curve on high k values
	template<typename Float>
	Float softclipFuzzExponential(Float, Float, Float) noexcept;

	// sample, ceiling, k
	// looks mostly linear, still smooth around ceil
	// 1 <= k.
	// k around Pi = pretty sharp already
	template<typename Float>
	Float softclipFiresledge(Float, Float, Float) noexcept;

	//sample, ceiling, k[0,1]
	//iconic softclipper "heavy" of unplug.red prisma
	template<typename Float>
	Float softclipPrismaHeavy(Float, Float, Float) noexcept;
}

/*

still have to implement this one: https://www.desmos.com/calculator/j3ynx47urc

use fireledge for the clipping parameter!

*/