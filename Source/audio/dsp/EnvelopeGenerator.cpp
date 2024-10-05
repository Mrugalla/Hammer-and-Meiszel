#include "EnvelopeGenerator.h"
#include <cmath>
#include "../../arch/Math.h"

namespace dsp
{
	EnvelopeGenerator::Parameters::Parameters(double _atk, double _dcy, double _sus, double _rls) :
		sampleRate(1.),
		atkP(_atk), dcyP(_dcy), rlsP(_rls),
		atk(math::msToInc(atkP, sampleRate)),
		dcy(math::msToInc(dcyP, sampleRate)),
		sus(_sus),
		rls(math::msToInc(rlsP, sampleRate))
	{
	}

	void EnvelopeGenerator::Parameters::prepare(double _sampleRate) noexcept
	{
		sampleRate = _sampleRate;
		atk = atkP;
		dcy = dcyP;
		rls = rlsP;
		atkP = dcyP = rlsP = -1.;
		operator()(atk, dcy, sus, rls);
	}

	void EnvelopeGenerator::Parameters::operator()(double _atk, double _dcy,
		double _sus, double _rls) noexcept
	{
		sus = _sus;
		if (atkP != _atk)
		{
			atkP = _atk;
			if (atkP < .1)
				atk = 1.;
			else
				atk = math::msToInc(atkP, sampleRate);
		}
		if (dcyP != _dcy)
		{
			dcyP = _dcy;
			if (dcyP < .1)
				dcy = 1.;
			else
				dcy = math::msToInc(dcyP, sampleRate);
		}
		if (rlsP != _rls)
		{
			rlsP = _rls;
			if (rlsP < .1)
				rls = 1.;
			else
				rls = math::msToInc(rlsP, sampleRate);
		}
	}

	void EnvelopeGenerator::Parameters::operator()(const Parameters& other) noexcept
	{
		operator()(other.atkP, other.dcyP, other.sus, other.rlsP);
	}

	bool EnvelopeGenerator::operator()(const MidiBuffer& midi, double* buffer, int numSamples) noexcept
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

	bool EnvelopeGenerator::isSleepy() noexcept
	{
		bool sleepy = !noteOn && curEnv < MinDb;
		if (sleepy)
			env = curEnv = 0.;
		return sleepy;
	}

	EnvelopeGenerator::EnvelopeGenerator(const Parameters& p) :
		parameters(p),
		env(0.), sampleRate(1.),
		state(State::Release),
		noteOn(false),
		smooth(0.),
		curEnv(0.),
		MinDb(math::dbToAmp(-60.))
	{
	}

	void EnvelopeGenerator::prepare(double _sampleRate) noexcept
	{
		sampleRate = _sampleRate;
		smooth.makeFromDecayInMs(.5, sampleRate);
	}

	double EnvelopeGenerator::operator()(bool _noteOn) noexcept
	{
		noteOn = _noteOn;
		return operator()();
	}

	double EnvelopeGenerator::operator()() noexcept
	{
		switch (state)
		{
		case State::Attack: processAttack(); break;
		case State::Decay: processDecay(); break;
		case State::Sustain: processSustain(); break;
		case State::Release:
		default: processRelease();
			break;
		}

		curEnv = smooth(env * env);
		return curEnv;
	}

	double EnvelopeGenerator::getEnvNoSustain() const noexcept
	{
		if (env < parameters.sus)
			return 0.;
		const auto a = env - parameters.sus;
		const auto b = 1. - parameters.sus;
		const auto y = a / b;
		return y * y;
	}

	void EnvelopeGenerator::processAttack() noexcept
	{
		if (noteOn)
		{
			env += parameters.atk * (1. - env);
			if (env > 1. - 1e-6)
			{
				env = 1.;
				state = State::Decay;
				processDecay();
			}
			return;
		}
		state = State::Release;
		processRelease();
	}

	void EnvelopeGenerator::processDecay() noexcept
	{
		if (noteOn)
		{
			env += parameters.dcy * (parameters.sus - env);
			const auto dist = parameters.sus - env;
			const auto distSquared = dist * dist;
			if (distSquared < 1e-6)
			{
				state = State::Sustain;
				processSustain();
			}
			return;
		}
		state = State::Release;
		processRelease();
	}

	void EnvelopeGenerator::processSustain() noexcept
	{
		if (noteOn)
		{
			env = parameters.sus;
			return;
		}
		state = State::Release;
		processRelease();
	}

	void EnvelopeGenerator::processRelease() noexcept
	{
		if (noteOn)
		{
			state = State::Attack;
			return processAttack();
		}
		env += parameters.rls * (0. - env);
		if (env < 1e-6)
			env = 0.;
	}

	int EnvelopeGenerator::synthesizeEnvelope(double* buffer, int s, int ts) noexcept
	{
		while (s < ts)
		{
			buffer[s] = operator()();
			++s;
		}
		return s;
	}

	//

	double EnvGenMultiVoice::Info::operator[](int i) const noexcept
	{
		return data[i];
	}

	EnvGenMultiVoice::EnvGenMultiVoice() :
		params(),
		envGens
		{
			params, params, params, params, params,
			params, params, params, params, params,
			params, params, params, params, params
		},
		buffer()
	{}

	void EnvGenMultiVoice::prepare(double sampleRate)
	{
		params.prepare(sampleRate);
		for (auto& envGen : envGens)
			envGen.prepare(sampleRate);
	}

	bool EnvGenMultiVoice::isSleepy(int vIdx) noexcept
	{
		return envGens[vIdx].isSleepy();
	}

	EnvGenMultiVoice::Info EnvGenMultiVoice::operator()(const MidiBuffer& midi, int numSamples, int vIdx) noexcept
	{
		auto bufferData = buffer.data();
		auto& envGen = envGens[vIdx];
		const auto active = envGen(midi, bufferData, numSamples);
		return { bufferData, active };
	}

	void EnvGenMultiVoice::updateParameters(const EnvelopeGenerator::Parameters& _params) noexcept
	{
		params(_params);
	}

	const EnvelopeGenerator::Parameters& EnvGenMultiVoice::getParameters() const noexcept
	{
		return params;
	}
}