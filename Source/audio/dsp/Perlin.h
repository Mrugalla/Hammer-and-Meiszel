#pragma once
#include <random>
#include "Transport.h"
#include "PRM.h"
#include "Phasor.h"
#include "XFade.h"

namespace perlin
{
	static constexpr int NumOctaves = 8;
	static constexpr int NoiseOvershoot = 4;

	static constexpr int NoiseSize = 1 << NumOctaves;
	static constexpr int NoiseSizeMax = NoiseSize - 1;
	static constexpr int BlockSize = dsp::BlockSize;
	using InterpolationFunc = double(*)(const double*, double) noexcept;
	using InterpolationFuncs = std::array<InterpolationFunc, 3>;
	using PRMInfo = dsp::PRMInfoD;
	using Phasor = dsp::PhasorD;
	using SIMD = dsp::SIMD;

	using NoiseArray = std::array<double, NoiseSize + NoiseOvershoot>;
	using GainBuffer = std::array<double, NumOctaves + 2>;

	enum class Shape
	{
		NN, Lerp, Spline, NumShapes
	};

	struct Perlin
	{
		Perlin();

		// sampleRate
		void prepare(double) noexcept;

		// newPhase
		void updatePosition(double) noexcept;

		// rateHzInv
		void updateSpeed(double) noexcept;

		//samples, noise, gainBuffer,
		//octavesInfo, phsInfo, widthInfo,
		//shape, numChannels, numSamples
		void operator()(double* const*, const double*, const double*,
			const PRMInfo&, const PRMInfo&, const PRMInfo&,
			Shape, int, int) noexcept;

		//samples, noise, gainBuffer,
		//octavesInfo, phsInfo, widthInfo,
		//shape, numChannels, numSamples
		void operator()(double*, const double*, const double*,
			const PRMInfo&, Shape, int) noexcept;

		//samples, noise, gainBuffer,
		//octavesInfo, phsInfo, widthInfo,
		//shape, numChannels, numSamples
		void operator()(double*, const double*, const double*,
			const PRMInfo&, double, int) noexcept;

		InterpolationFuncs interpolationFuncs;
		double sampleRateInv, sampleRate;
		Phasor phasor;
		std::array<double, BlockSize> phaseBuffer;
		int noiseIdx;
	private:
		// phsInfo, numSamples
		void synthesizePhasor(const PRMInfo&, int) noexcept;

		// numSamples
		void synthesizePhasor(int) noexcept;

		double getInterpolatedSample(const double*,
			double, Shape) const noexcept;

		double getInterpolatedSample(const double*,
			double, double) const noexcept;

		// smpls, octavesInfo, noise, gainBuffer, shape, numSamples
		void processOctaves(double*, const PRMInfo&,
			const double*, const double*, Shape, int) noexcept;

		// smpls, octavesInfo, noise, gainBuffer, shape, numSamples
		void processOctaves(double*, const PRMInfo&,
			const double*, const double*, double, int) noexcept;

		// smpls, noise, gainBuffer, octaves, shape, numSamples
		void processOctavesNotSmoothing(double*, const double*,
			const double*, double,
			Shape, int) noexcept;

		// smpls, noise, gainBuffer, octaves, shape, numSamples
		void processOctavesNotSmoothing(double*, const double*,
			const double*, double,
			double, int) noexcept;

		// smpls, octavesBuf, noise, gainBuffer, shape, numSamples
		void processOctavesSmoothing(double*, const double*,
			const double*, const double*,
			Shape, int) noexcept;

		// smpls, octavesBuf, noise, gainBuffer, shape, numSamples
		void processOctavesSmoothing(double*, const double*,
			const double*, const double*,
			double, int) noexcept;

		// samples, octavesInfo, widthInfo, noise, gainBuffer, shape, numSamples
		void processWidth(double* const*, const PRMInfo&,
			const PRMInfo&, const double*, const double*,
			Shape, int) noexcept;

		// samples, octavesInfo, widthInfo, noise, gainBuffer, shape, numSamples
		void processWidth(double* const*, const PRMInfo&,
			const PRMInfo&, const double*, const double*,
			double, int) noexcept;

		// phaseInfo, o
		double getPhaseOctaved(double, int) const noexcept;

		// debug:
#if JUCE_DEBUG
		// smpls, numSamples, threshold
		void discontinuityJassert(double*, int, double = .1);

		// samples, numChannels, numSamples
		void controlRange(double* const*, int, int) noexcept;
#endif
	};

	static constexpr double XFadeLengthMs = 200.;
	static constexpr int NumPerlins = 3;

	using PRM = dsp::PRMD;
	using Mixer = dsp::XFadeMixer<NumPerlins, true>;
	using Perlins = std::array<Perlin, NumPerlins>;
	using Int64 = juce::int64;
	static constexpr double Pi = dsp::Pi;
	using Transport = dsp::Transport::Info;

	struct Perlin2
	{
		Perlin2();

		void setSeed(int);

		// sampleRate
		void prepare(double);

		// samples, numChannels, numSamples, transport,
		// rateHz, rateBeats, octaves, width, phs, bias[0,1]
		// shape, temposync
		void operator()(double**, int, int,
			const Transport&,
			double, double,
			double, double, double, double,
			Shape, bool) noexcept;

		// smpls, numSamples, transport,
		// rateBeats, octaves, bias[0,1], shape
		void operator()(double*, int,
			const Transport&,
			double, double, double,
			Shape) noexcept;

		// smpls, numSamples, transport,
		// rateBeats, octaves, bias[0,1], shape
		void operator()(double*, int,
			const Transport&,
			double, double, double,
			double) noexcept;

		Mixer mixer;
		// misc
		double sampleRateInv;
		// noise
		NoiseArray noise;
		GainBuffer gainBuffer, gainBufferOct;
		Perlins perlins;
		// parameters
		PRM octavesPRM, widthPRM, phsPRM;
		double rateBeats, rateHz;
		double inc, bpm, bps, rateInv;
		// seed
		std::atomic<int> seed;
		// project position
		Int64 posEstimate;

		// transport, rateBeats, rateHz, timeInSamples, temposync
		void updatePerlin(const Transport&,
			double, double, int, bool) noexcept;

		// transport, rateBeats, numSamples
		void updatePerlin(const Transport&,
			double, int) noexcept;

		// bpm, rateHz, rateBeats, timeInSamples, temposync
		void updateSpeed(double, double, double, Int64, bool) noexcept;

		// bpm, rateBeats, timeInSamples
		void updateSpeed(double, double, Int64) noexcept;

		// perlin, ppq, timeSecs, temposync
		void updatePosition(Perlin&, double, double, bool) noexcept;

		// perlin, ppq
		void updatePositionSync(Perlin&, double) noexcept;

		// CROSSFADE FUNCS
		// timeSamples
		bool isLooping(Int64) noexcept;

		// bpm, inc
		const bool keepsSpeed(double, double) const noexcept;

		// bpm, inc
		const bool changesSpeed(double, double) const noexcept;

		// inc, bpm, bps, rateInv, rateHz, rateBeats
		void initXFade(double, double, double,
			double, double, double) noexcept;

		// samples, bias, numChannels, numSamples
		void processBias(double* const*, double,
			int, int) noexcept;

		// smpls, bias, numSamples
		void processBias(double*, double,
			int) noexcept;

		// samples, octaves, numChannels, numSamples
		void fuckingApplyGainMate(double**, double,
			int, int) noexcept;

		// smpls, octaves, numSamples
		void fuckingApplyGainMate(double*, double, int) noexcept;
	};
}