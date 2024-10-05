#pragma once
#include "Smooth.h"

namespace dsp
{
	struct EnvelopeGenerator
	{
		enum class State
		{
			Attack,
			Decay,
			Sustain,
			Release
		};

		struct Parameters
		{
			// atkMs, dcyMs, sus, rlsMs
			Parameters(double = 2., double = 20., double = .8, double = 10.);

			// sampleRate
			void prepare(double) noexcept;

			// atkMs, dcyMs, sus, rlsMs
			void operator()(double, double, double, double) noexcept;

			void operator()(const Parameters&) noexcept;

			double sampleRate, atkP, dcyP, rlsP;
			double atk, dcy, sus, rls;
		};

		EnvelopeGenerator(const Parameters&);

		// sampleRate
		void prepare(double) noexcept;

		bool isSleepy() noexcept;

		// returns true if envelope active
		// midi, buffer, numSamples
		bool operator()(const MidiBuffer&, double*, int) noexcept;

		double operator()(bool) noexcept;

		double operator()() noexcept;
		
		// don't use this if sustain can equal 1
		double getEnvNoSustain() const noexcept;

		const Parameters& parameters;
		double env, sampleRate;
		State state;
		bool noteOn;
	protected:
		smooth::LowpassD smooth;
		double curEnv;
		const double MinDb;

		void processAttack() noexcept;

		void processDecay() noexcept;

		void processSustain() noexcept;

		void processRelease() noexcept;

		// buffer, s, ts
		int synthesizeEnvelope(double*, int, int) noexcept;
	};

	struct EnvGenMultiVoice
	{
		struct Info
		{
			double operator[](int i) const noexcept;

			double* data;
			const bool active;
		};

		EnvGenMultiVoice();

		// sampleRate
		void prepare(double);

		// vIdx
		bool isSleepy(int) noexcept;

		// midi, numSamples, vIdx
		Info operator()(const MidiBuffer&, int, int) noexcept;

		void updateParameters(const EnvelopeGenerator::Parameters&) noexcept;

		const EnvelopeGenerator::Parameters& getParameters() const noexcept;

	protected:
		EnvelopeGenerator::Parameters params;
		std::array<EnvelopeGenerator, NumMPEChannels> envGens;
		std::array<double, BlockSize> buffer;
	};
}