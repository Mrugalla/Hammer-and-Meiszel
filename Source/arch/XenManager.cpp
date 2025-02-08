#include "XenManager.h"
#include "Math.h"
#include <cmath>

namespace arch
{
	bool XenManager::Info::operator==(const Info& other) const noexcept
	{
		return xen == other.xen && masterTune == other.masterTune &&
			anchor == other.anchor && pitchbendRange == other.pitchbendRange;
	}

	XenManager::XenManager() :
		info{ 12., 440., 69., 2. }
	{
	}

	void XenManager::operator()(double _xen, double _masterTune,
		double _anchor, double _pitchbendRange) noexcept
	{
		info.xen = _xen;
		info.masterTune = _masterTune;
		info.anchor = _anchor;
		info.pitchbendRange = _pitchbendRange;
	}

	template<typename Float>
	Float XenManager::noteToFreqHz(Float note) const noexcept
	{
		return math::noteToFreqHz
		(
			note,
			static_cast<Float>(info.anchor),
			static_cast<Float>(info.xen),
			static_cast<Float>(info.masterTune)
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
			static_cast<Float>(info.anchor),
			static_cast<Float>(info.xen),
			static_cast<Float>(info.masterTune)
		);
	}

	double XenManager::getXen() const noexcept
	{
		return info.xen;
	}

	double XenManager::getPitchbendRange() const noexcept
	{
		return info.pitchbendRange;
	}

	double XenManager::getAnchor() const noexcept
	{
		return info.anchor;
	}

	double XenManager::getMasterTune() const noexcept
	{
		return info.masterTune;
	}

	const XenManager::Info& XenManager::getInfo() const noexcept
	{
		return info;
	}

	template float XenManager::noteToFreqHz<float>(float note) const noexcept;
	template double XenManager::noteToFreqHz<double>(double note) const noexcept;

	template float XenManager::noteToFreqHzWithWrap<float>(float note, float lowestFreq, float highestFreq) const noexcept;
	template double XenManager::noteToFreqHzWithWrap<double>(double note, double lowestFreq, double highestFreq) const noexcept;

	template float XenManager::freqHzToNote<float>(float hz) const noexcept;
	template double XenManager::freqHzToNote<double>(double hz) const noexcept;
}