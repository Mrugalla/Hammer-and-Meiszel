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

		EnvelopeGenerator();

		// sampleRate
		void prepare(double) noexcept;

		// atkMs, dcyMs, sus, rlsMs
		void updateParameters(double, double, double, double) noexcept;

		double operator()(bool) noexcept;

		double operator()() noexcept;

		double atkP, dcyP, rlsP, sampleRate;
		double env, atk, dcy, sus, rls;
		State state;
		bool noteOn;
	protected:
		smooth::LowpassD smooth;

		void processAttack() noexcept;

		void processDecay() noexcept;

		void processSustain() noexcept;

		void processRelease() noexcept;
	};
}