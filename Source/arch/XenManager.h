#pragma once
#include <array>
#include <atomic>

namespace arch
{
	struct XenManager
	{
		struct Info
		{
			bool operator==(const Info&) const noexcept;

			double xen, masterTune, anchor, pitchbendRange;
		};

		XenManager();

		// xen, masterTune, anchor, pitchbendRange
		void operator()(double, double, double, double) noexcept;

		template<typename Float>
		Float noteToFreqHz(Float) const noexcept;

		// note, lowestFreq, highestFreq
		template<typename Float>
		Float noteToFreqHzWithWrap(Float, Float = static_cast<Float>(0), Float = static_cast<Float>(22049)) const noexcept;

		template<typename Float>
		Float freqHzToNote(Float) const noexcept;

		const Info& getInfo() const noexcept;

		double getXen() const noexcept;

		double getPitchbendRange() const noexcept;

		double getAnchor() const noexcept;

		double getMasterTune() const noexcept;

	protected:
		Info info;
	};
}