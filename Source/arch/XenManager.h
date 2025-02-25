#pragma once
#include <array>
#include <atomic>
#include <functional>

namespace arch
{
	struct XenManager
	{
		struct Info
		{
			bool operator==(const Info&) const noexcept;

			double xen, masterTune, anchor, pitchbendRange;
		};

		// xenInfo, numChannels
		using UpdateFunc = std::function<void(const Info&, int)>;

		XenManager();

		// xenInfo, numChannels
		void operator()(const Info&, int) noexcept;

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

		UpdateFunc updateFunc;
	protected:
		Info info;
	};
}