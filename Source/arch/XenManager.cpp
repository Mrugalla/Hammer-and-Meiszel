#include "XenManager.h"
#include "Math.h"
#include <cmath>

namespace arch
{
	XenManager::XenManager() :
		xen(12.),
		masterTune(440.),
		anchor(69.),
		pitchbendRange(2.)
	{
	}

	void XenManager::operator()(double _xen, double _masterTune,
		double _anchor, double _pitchbendRange) noexcept
	{
		xen = _xen;
		masterTune = _masterTune;
		anchor = _anchor;
		pitchbendRange = _pitchbendRange;
	}

	template<typename Float>
	Float XenManager::noteToFreqHz(Float note) const noexcept
	{
		return math::noteToFreqHz
		(
			note,
			static_cast<Float>(anchor),
			static_cast<Float>(xen),
			static_cast<Float>(masterTune)
		);
	}

	template<typename Float>
	Float XenManager::noteToFreqHzWithWrap(Float note, Float lowestFreq, Float highestFreq) const noexcept
	{
		auto freq = noteToFreqHz(note);
		while (freq < lowestFreq)
			freq *= static_cast<Float>(2);
		while (freq >= highestFreq)
			freq *= static_cast<Float>(.5);
		return freq;
	}

	template<typename Float>
	Float XenManager::freqHzToNote(Float hz) const noexcept
	{
		return math::freqHzToNote
		(
			hz,
			static_cast<Float>(anchor),
			static_cast<Float>(xen),
			static_cast<Float>(masterTune)
		);
	}

	double XenManager::getXen() const noexcept
	{
		return xen;
	}

	double XenManager::getPitchbendRange() const noexcept
	{
		return pitchbendRange;
	}

	double XenManager::getAnchor() const noexcept
	{
		return anchor;
	}

	double XenManager::getMasterTune() const noexcept
	{
		return masterTune;
	}

	template float XenManager::noteToFreqHz<float>(float note) const noexcept;
	template double XenManager::noteToFreqHz<double>(double note) const noexcept;

	template float XenManager::noteToFreqHzWithWrap<float>(float note, float lowestFreq, float highestFreq) const noexcept;
	template double XenManager::noteToFreqHzWithWrap<double>(double note, double lowestFreq, double highestFreq) const noexcept;

	template float XenManager::freqHzToNote<float>(float hz) const noexcept;
	template double XenManager::freqHzToNote<double>(double hz) const noexcept;
}