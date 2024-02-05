#pragma once
#include <array>
#include <atomic>

namespace arch
{
	struct XenManager
	{
		XenManager();

		/* xen, masterTune, referencePitch, pitchbendRange */
		void operator()(double, double, double, double) noexcept;

		template<typename Float>
		Float noteToFreqHz(Float) const noexcept;

		/* note, lowestFreq, highestFreq */
		template<typename Float>
		Float noteToFreqHzWithWrap(Float, Float = static_cast<Float>(0), Float = static_cast<Float>(22049)) const noexcept;

		template<typename Float>
		Float freqHzToNote(Float) const noexcept;

		double getXen() const noexcept;

	protected:
		double xen, masterTune, referencePitch, pitchbendRange;
	};
}