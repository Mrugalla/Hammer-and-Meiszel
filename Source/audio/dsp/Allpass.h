#pragma once
#include <array>

namespace dsp
{
	static constexpr int NumAllpassFilters = 128;

	
	//Canonical Direct Form II (TDF-II)
	//no bandwidth
	struct AllpassFirstOrder
	{
		AllpassFirstOrder();

		void reset() noexcept;

		void copyFrom(const AllpassFirstOrder&) noexcept;

		// fc = pi * freq / sampleRate
		void setFreqFc(double) noexcept;

		double operator()(double) noexcept;
	private:
		double y1, g;
	};

	//Canonical Form II 2nd Order (TDF-II)
	//bad modulation
	struct AllpassTransposedDirectFormII
	{
		AllpassTransposedDirectFormII();

		void reset() noexcept;

		void copyFrom(const AllpassTransposedDirectFormII&) noexcept;

		// fc = pi * freqHz / sampleRate
		void setFreqFc(double) noexcept;

		// qHz
		void setQ(double) noexcept;

		void update() noexcept;

		double operator()(double) noexcept;

	private:
		double k, kk, kq, norm;
		double a0, a1, a2, b1, b2;
		double z1, z2;
	};

	//Biquad 2nd Order Direct Form I
	//no bandwidth
	//bad performance
	//bad modulation
	//numeric instability
	//fixed-point friendly
	struct Allpass2ndOrderDirectFormI
	{
		Allpass2ndOrderDirectFormI();

		void reset() noexcept;

		void copyFrom(const Allpass2ndOrderDirectFormI&) noexcept;

		// freqHz, sampleRate
		void updateParameters(double, double) noexcept;

		double operator()(double) noexcept;

		double a0, a1, a2, b1, b2;
	private:
		double x1, x2, y1, y2;
	};

	///Biquad 2nd Order Direct Form I
	//bad performance
	//bad modulation
	//numeric instability
	//fixed-point friendly
	struct Allpass2ndOrderDirectFormIBW
	{
		Allpass2ndOrderDirectFormIBW();

		void reset() noexcept;

		void copyFrom(const Allpass2ndOrderDirectFormIBW&) noexcept;

		// freqHz, qHz, sampleRate
		void updateParameters(double, double, double) noexcept;

		double operator()(double) noexcept;

		double a0, a1, a2, b1, b2;
	private:
		double x1, x2, y1, y2;
	};

	/*
	2 channels of AllpassTransposedDirectFormII filters
	*/
	struct AllpassStereo
	{
		AllpassStereo();

		void reset() noexcept;

		void copyFrom(const AllpassStereo&) noexcept;

		// freqHz, ch
		void setFreqFc(double, int) noexcept;

		// qHz, ch
		void setQ(double, int) noexcept;

		// ch
		void update(int) noexcept;

		void copyLeftToRight() noexcept;
		void copyRightToLeft() noexcept;

		// smpl, ch
		double operator()(double, int) noexcept;

	private:
		std::array<AllpassTransposedDirectFormII, 2> filters;
	};

	/*
	axiom::NumAllpassFilters channels of AllpassTransposedDirectFormII filters
	*/
	struct AllpassSlope
	{
		AllpassSlope();

		void reset() noexcept;

		/* freqHz, qHz, sampleRate, numFilters */
		void updateParameters(double, double, double, int) noexcept;

		/* other, numFilters */
		void copyFrom(const AllpassSlope&, int) noexcept;

		/* smpl */
		double operator()(double) noexcept;

	private:
		std::array<AllpassTransposedDirectFormII, NumAllpassFilters> allpasses;
		int numFilters;
	};

	//NumAllpassFilters channels of AllpassStereo filters
	struct AllpassSlopeStereo
	{
		AllpassSlopeStereo();

		void reset() noexcept;

		// freqFc [0, pi], ch, filterIdx
		void setFreqFc(double, int, int) noexcept;

		// qHz, ch, filterIdx
		void setQ(double, int, int) noexcept;

		// numFilters
		void setNumFilters(int) noexcept;

		// ch, filterIdx
		void update(int, int) noexcept;

		// filterIdx
		void copyLeftToRight(int) noexcept;

		// filterIdx
		void copyRightToLeft(int) noexcept;

		// filterIdx
		void copyToAllFilters(int) noexcept;

		// smpl, ch
		double operator()(double, int) noexcept;

	private:
		std::array<AllpassStereo, NumAllpassFilters> allpasses;
		int numFilters;
	};

	/*
	2 channels of AllpassSlope filters
	*/
	struct AllpassStereoSlope
	{
		AllpassStereoSlope();

		void reset() noexcept;

		/* freqHzL, freqHzR, qHzL, qHzR, sampleRate, numFiltersLeft, numFiltersRight */
		void updateParameters(double, double, double, double, double, int, int) noexcept;

		/* smpl, ch */
		double operator()(double, int) noexcept;

	private:
		std::array<AllpassSlope, 2> allpasses;
	};
}

/*
todo:

some allpass base classes still have a single updateParameters method

*/