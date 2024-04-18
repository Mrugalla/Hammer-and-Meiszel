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
			Parameters();

			// sampleRate
			void prepare(double) noexcept;

			// atkMs, dcyMs, sus, rlsMs
			void updateParameters(double, double, double, double) noexcept;

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

		const Parameters& parameters;
		double env, sampleRate;
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