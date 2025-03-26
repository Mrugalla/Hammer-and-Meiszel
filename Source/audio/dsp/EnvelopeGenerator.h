#pragma once
#include "../Using.h"

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
			void processMs(double, double, double, double) noexcept;

			// atkTS, dcyTS, sus, rlsTS, bpm
			void processSync(double, double, double, double, double) noexcept;

			double sampleRate, atkP, dcyP, rlsP;
			double atk, dcy, sus, rls;
		};

		EnvelopeGenerator(const Parameters&);

		// sampleRate
		void prepare(double) noexcept;

		bool isSleepy() const noexcept;

		// returns true if envelope active
		// midi, buffer, numSamples
		bool operator()(const MidiBuffer&, double*, int) noexcept;

		double operator()(bool) noexcept;

		double operator()() noexcept;

		const Parameters& parameters;
		double env, sampleRate;
		State state;
		bool noteOn;
	protected:
		double phase, envStart;
		const double MinDb;

		void triggerAttackState() noexcept;

		void triggerDecayState() noexcept;

		void triggerReleaseState() noexcept;

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
		bool isSleepy(int) const noexcept;

		// samplesOut, samplesIn, numChannels, numSamples, v
		// returns true if envelope active
		bool processGain(double**, const double**, int, int, int) noexcept;

		// samples, numChannels, numSamples, v
		// returns true if envelope active
		bool processGain(double**, int, int, int) noexcept;

		// midi, numSamples, vIdx
		Info operator()(const MidiBuffer&, int, int) noexcept;

		// vIdx, numSamples
		Info operator()(int, int) noexcept;

		void triggerNoteOn(bool e, int vIdx) noexcept
		{
			envGens[vIdx].noteOn = e;
		}

		void updateParametersMs(const EnvelopeGenerator::Parameters&) noexcept;

		void updateParametersSync(const EnvelopeGenerator::Parameters&, double bpm) noexcept;

		const EnvelopeGenerator::Parameters& getParameters() const noexcept;

	protected:
		EnvelopeGenerator::Parameters params;
		std::array<EnvelopeGenerator, NumMPEChannels> envGens;
		std::array<double, BlockSize> buffer;
	};
}