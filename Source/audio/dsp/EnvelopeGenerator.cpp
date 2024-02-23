#include "EnvelopeGenerator.h"
#include <cmath>
#include "../../arch/Math.h"

namespace dsp
{
	EnvelopeGenerator::EnvelopeGenerator() :
		env(0.),
		atkP(2.), dcyP(20.), rlsP(10.), sampleRate(44100.),
		atk(math::msToInc(atkP, sampleRate)),
		dcy(math::msToInc(dcyP, sampleRate)),
		sus(.8),
		rls(math::msToInc(rlsP, sampleRate)),
		state(State::Release),
		noteOn(false),
		smooth(0.)
	{
	}

	void EnvelopeGenerator::prepare(double _sampleRate) noexcept
	{
		sampleRate = _sampleRate;
		smooth.makeFromDecayInMs(20., sampleRate);
	}

	void EnvelopeGenerator::updateParameters(double _atk, double _dcy,
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
		default: processRelease(); break;
		}

		return smooth(env * env);
	}

	void EnvelopeGenerator::processAttack() noexcept
	{
		if (noteOn)
		{
			env += atk * (1. - env);
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
			env += dcy * (sus - env);
			if (std::abs(sus - env) < 1e-6)
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
			env = sus;
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
		env += rls * (0. - env);
		if (env < 1e-6)
			env = 0.;
	}
}