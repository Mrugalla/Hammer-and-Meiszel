#include "EnvelopeGenerator.h"
#include <cmath>
#include "../../arch/Math.h"

namespace dsp
{
	EnvelopeGenerator::Parameters::Parameters(double _atk, double _dcy, double _sus, double _rls) :
		sampleRate(1.),
		atkP(_atk), dcyP(_dcy), rlsP(_rls),
		atk(-1.),
		dcy(-1.),
		sus(_sus),
		rls(-1.)
	{
	}

	void EnvelopeGenerator::Parameters::prepare(double _sampleRate) noexcept
	{
		sampleRate = _sampleRate;
		atkP = dcyP = rlsP = -1.;
	}

	void EnvelopeGenerator::Parameters::processSync(double _atk, double _dcy,
		double _sus, double _rls, double bpm) noexcept
	{
		sus = _sus;
		const auto timeVal = 60000. / bpm * 4.;
		if (atkP != _atk)
		{
			atkP = _atk;
			if (atkP != 0.)
			{
				const auto atkMs = timeVal * atkP;
				atk = math::msToInc(atkMs, sampleRate);
			}
			else
				atk = 1.;
		}
		if (dcyP != _dcy)
		{
			dcyP = _dcy;
			if (dcyP != 0.)
			{
				const auto dcyMs = timeVal * dcyP;
				dcy = math::msToInc(dcyMs, sampleRate);
			}
			else
				dcy = 1.;
		}
		if (rlsP != _rls)
		{
			rlsP = _rls;
			if (rlsP != 0.)
			{
				const auto rlsMs = timeVal * rlsP;
				rls = math::msToInc(rlsMs, sampleRate);
			}
			else
				rls = 1.;
		}
	}

	void EnvelopeGenerator::Parameters::processMs(double _atk, double _dcy,
		double _sus, double _rls) noexcept
	{
		sus = _sus;
		if (atkP != _atk)
		{
			atkP = _atk;
			if (atkP != 0.)
				atk = math::msToInc(atkP, sampleRate);
			else
				atk = 1.;
		}
		if (dcyP != _dcy)
		{
			dcyP = _dcy;
			if (dcyP != 0.)
				dcy = math::msToInc(dcyP, sampleRate);
			else
				dcy = 1.;
		}
		if (rlsP != _rls)
		{
			rlsP = _rls;
			if (rlsP != 0.)
				rls = math::msToInc(rlsP, sampleRate);
			else
				rls = 1.;
		}
	}

	bool EnvelopeGenerator::operator()(const MidiBuffer& midi, double* buffer, int numSamples) noexcept
	{
		if (midi.isEmpty() && isSleepy())
			return false;

		auto start = 0;
		for (const auto it : midi)
		{
			const auto msg = it.getMessage();
			const auto end = it.samplePosition;
			start = synthesizeEnvelope(buffer, start, end);
			if (msg.isNoteOn())
				noteOn = true;
			else if (msg.isNoteOff() || msg.isAllNotesOff())
				noteOn = false;
		}
		synthesizeEnvelope(buffer, start, numSamples);

		return true;
	}

	bool EnvelopeGenerator::isSleepy() const noexcept
	{
		return !noteOn && env < MinDb;
	}

	EnvelopeGenerator::EnvelopeGenerator(const Parameters& p) :
		parameters(p),
		env(0.), sampleRate(1.),
		state(State::Release),
		noteOn(false),
		phase(1.), envStart(0.),
		MinDb(math::dbToAmp(-60.))
	{
	}

	void EnvelopeGenerator::prepare(double _sampleRate) noexcept
	{
		sampleRate = _sampleRate;
		phase = 1.;
		envStart = 0.;
		env = 0.;
		state = State::Release;
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
		return env;
	}

	void EnvelopeGenerator::triggerAttackState() noexcept
	{
		state = State::Attack;
		envStart = env;
		phase = 0.;
		processAttack();
	}

	void EnvelopeGenerator::triggerDecayState() noexcept
	{
		state = State::Decay;
		phase = 0.;
		env = 1.;
		processDecay();
	}

	void EnvelopeGenerator::triggerReleaseState() noexcept
	{
		state = State::Release;
		envStart = env;
		phase = 0.;
		processRelease();
	}

	void EnvelopeGenerator::processAttack() noexcept
	{
		if (!noteOn)
			return triggerReleaseState();
		const auto x = dsp::Pi + phase * dsp::Pi;
		const auto w = .5 * math::cosApprox(x > Pi ? x - Tau : x) + .5;
		env = envStart + w * (1. - envStart);
		phase += parameters.atk;
		if (phase < 1.)
			return;
		triggerDecayState();
	}

	void EnvelopeGenerator::processDecay() noexcept
	{
		if (!noteOn)
			return triggerReleaseState();
		const auto x = phase * dsp::Pi;
		const auto w = .5 * math::cosApprox(x) + .5;
		env = parameters.sus + w * (1. - parameters.sus);
		phase += parameters.dcy;
		if (phase < 1.)
			return;
		state = State::Sustain;
		env = parameters.sus;
		processSustain();
	}

	void EnvelopeGenerator::processSustain() noexcept
	{
		if (!noteOn)
			return triggerReleaseState();
		env = parameters.sus;
	}

	void EnvelopeGenerator::processRelease() noexcept
	{
		if (noteOn)
			return triggerAttackState();
		if (phase >= 1.)
			return;
		const auto x = phase * dsp::Pi;
		const auto w = .5 * math::cosApprox(x) + .5;
		env = envStart * w;
		phase += parameters.rls;
		if (phase < 1.)
			return;
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

	bool EnvGenMultiVoice::isSleepy(int vIdx) const noexcept
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

	EnvGenMultiVoice::Info EnvGenMultiVoice::operator()(int vIdx, int numSamples) noexcept
	{
		auto bufferData = buffer.data();
		auto& envGen = envGens[vIdx];
		for (auto s = 0; s < numSamples; ++s)
		{
			const auto env = envGen();
			bufferData[s] = env;
		}
		return { bufferData, !envGen.isSleepy() };
	}

	bool EnvGenMultiVoice::processGain(double** samplesOut, const double** samplesIn,
		int numChannels, int numSamples, int v) noexcept
	{
		const auto info = operator()(v, numSamples);
		if (info.active)
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				dsp::SIMD::multiply(samplesOut[ch], samplesIn[ch], info.data, numSamples);
			return true;
		}
		for (auto ch = 0; ch < numChannels; ++ch)
			dsp::SIMD::clear(samplesOut[ch], numSamples);
		return false;
	}

	bool EnvGenMultiVoice::processGain(double** samples,
		int numChannels, int numSamples, int v) noexcept
	{
		const auto info = operator()(v, numSamples);
		if (info.active)
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				dsp::SIMD::multiply(samples[ch], info.data, numSamples);
			return true;
		}
		for (auto ch = 0; ch < numChannels; ++ch)
			dsp::SIMD::clear(samples[ch], numSamples);
		return false;
	}

	void EnvGenMultiVoice::updateParametersMs(const EnvelopeGenerator::Parameters& p) noexcept
	{
		params.processMs(p.atkP, p.dcyP, p.sus, p.rlsP);
	}

	void EnvGenMultiVoice::updateParametersSync(const EnvelopeGenerator::Parameters& p, double bpm) noexcept
	{
		params.processSync(p.atkP, p.dcyP, p.sus, p.rlsP, bpm);
	}

	const EnvelopeGenerator::Parameters& EnvGenMultiVoice::getParameters() const noexcept
	{
		return params;
	}
}