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
			void updateParameters(double, double, double, double) noexcept;

			void updateParameters(const Parameters&) noexcept;

		private:
			double sampleRate, atkP, dcyP, rlsP;
		public:
			double atk, dcy, sus, rls;
		};

		EnvelopeGenerator(const Parameters&);

		// sampleRate
		void prepare(double) noexcept;

		double operator()(bool) noexcept;

		double operator()() noexcept;

		double getEnvNoSustain() const noexcept;

		const Parameters& parameters;
		double env, sampleRate;
		State state;
		bool noteOn;
	protected:
		smooth::LowpassD smooth;
		double curEnv;

		void processAttack() noexcept;

		void processDecay() noexcept;

		void processSustain() noexcept;

		void processRelease() noexcept;
	};
}