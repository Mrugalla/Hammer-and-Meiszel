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
		private:
			double sampleRate, atkP, dcyP, rlsP;
		public:
			double atk, dcy, sus, rls;
		};

		EnvelopeGenerator(const Parameters&);

		// sampleRate
		void prepare(double) noexcept;

		const bool isSleepy() const noexcept
		{
			static constexpr auto Eps = 1e-6;
			return !noteOn && env < Eps;
		}

		// returns true if envelope active
		bool operator()(const MidiBuffer& midi, double* buffer, int numSamples) noexcept
		{
			if (midi.isEmpty() && isSleepy())
				return false;

			auto s = 0;
			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				const auto ts = it.samplePosition;
				if (msg.isNoteOn())
				{
					s = synthesizeEnvelope(buffer, s, ts);
					noteOn = true;
				}
				else if (msg.isNoteOff() || msg.isAllNotesOff())
				{
					s = synthesizeEnvelope(buffer, s, ts);
					noteOn = false;
				}
				else
					s = synthesizeEnvelope(buffer, s, ts);
			}
			
			synthesizeEnvelope(buffer, s, numSamples);
			
			return true;
		}

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

		void processAttack() noexcept;

		void processDecay() noexcept;

		void processSustain() noexcept;

		void processRelease() noexcept;

		int synthesizeEnvelope(double* buffer, int s, int ts) noexcept
		{
			while (s < ts)
			{
				buffer[s] = operator()();
				++s;
			}
			return s;
		}
	};

	struct EnvGenMultiVoice
	{
		struct Info
		{
			double* data;
			const bool active;
		};

		EnvGenMultiVoice() :
			params(),
			envGens
			{
				params, params, params, params, params,
				params, params, params, params, params,
				params, params, params, params, params
			},
			buffer()
		{}

		void prepare(double sampleRate)
		{
			params.prepare(sampleRate);
			for (auto& envGen : envGens)
				envGen.prepare(sampleRate);
		}

		bool isSleepy(int vIdx) const noexcept
		{
			return envGens[vIdx].isSleepy();
		}

		Info operator()(const MidiBuffer& midi, int numSamples, int vIdx) noexcept
		{
			auto bufferData = buffer.data();
			const auto active = envGens[vIdx](midi, bufferData, numSamples);
			return { bufferData, active };
		}

		void updateParameters(const EnvelopeGenerator::Parameters& _params) noexcept
		{
			params(_params);
		}

	protected:
		EnvelopeGenerator::Parameters params;
		std::array<EnvelopeGenerator, NumMPEChannels> envGens;
		std::array<double, BlockSize> buffer;
	};
}