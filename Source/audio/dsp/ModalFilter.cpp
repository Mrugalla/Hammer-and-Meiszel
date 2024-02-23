#include "ModalFilter.h"

namespace dsp
{
	namespace modal
	{
		// Resonator

		Resonator::Val::Val() :
			pitch(0.),
			transpose(0.),
			pb(0.),
			pbRange(2.),
			xen(12.)
		{}

		double Resonator::Val::getFreq(const arch::XenManager& _xen) noexcept
		{
			return _xen.noteToFreqHzWithWrap(pitch + transpose + pb * pbRange * 2.);
		}

		Resonator::Resonator(const arch::XenManager& _xen, const DualMaterial& _material) :
			xen(_xen),
			material(_material),
			resonators(),
			val(),
			freqHz(1000.),
			sampleRate(1.),
			nyquist(.5),
			numFiltersBelowNyquist(0)
		{
		}

		void Resonator::reset() noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
				resonators[i].reset();
		}

		void Resonator::prepare(double _sampleRate)
		{
			sampleRate = _sampleRate;
			nyquist = sampleRate * .5;
			setFrequencyHz(1000.);
		}

		void Resonator::operator()(double** samples, const MidiBuffer& midi,
			double transposeSemi, int numChannels, int numSamples) noexcept
		{
			if (val.transpose != transposeSemi ||
				val.pbRange != xen.getPitchbendRange() ||
				val.xen != xen.getXen())
			{
				val.transpose = transposeSemi;
				val.pbRange = xen.getPitchbendRange();
				val.xen = xen.getXen();
				const auto freq = val.getFreq(xen);
				setFrequencyHz(freq);
			}

			process(samples, midi, numChannels, numSamples);
		}

		void Resonator::setFrequencyHz(double freq) noexcept
		{
			if (freqHz == freq)
				return;
			freqHz = freq;
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto freqRatio = material.getRatio(i);
				const auto freqFilter = freqHz * freqRatio;
				if (freqFilter < nyquist)
				{
					const auto fc = math::freqHzToFc(freqFilter, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc);
					//resonator.reset();
					resonator.update();
				}
				else
				{
					numFiltersBelowNyquist = i;
					return;
				}
			}
			numFiltersBelowNyquist = NumFilters;
		}

		void Resonator::updateFreqRatios() noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto freqRatio = material.getRatio(i);
				const auto freqFilter = freqHz * freqRatio;
				if (freqFilter < nyquist)
				{
					const auto fc = math::freqHzToFc(freqFilter, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc);
					resonator.update();
				}
				else
				{
					numFiltersBelowNyquist = i;
					return;
				}
			}
			numFiltersBelowNyquist = NumFilters;
		}

		void Resonator::setBandwidth(double bw) noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				auto& resonator = resonators[i];
				resonator.setBandwidth(bw);
				resonator.update();
			}
		}

		void Resonator::process(double** samples, const MidiBuffer& midi, int numChannels, int numSamples) noexcept
		{
			static constexpr double PB = 0x3fff;
			static constexpr double PBInv = 1. / PB;

			auto s = 0;
			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					const auto ts = it.samplePosition;
					val.pitch = static_cast<double>(msg.getNoteNumber());
					const auto freq = val.getFreq(xen);
					setFrequencyHz(freq);
					applyFilter(samples, numChannels, s, ts);
					s = ts;
				}
				else if (msg.isPitchWheel())
				{
					const auto ts = it.samplePosition;
					const auto pb = static_cast<double>(msg.getPitchWheelValue()) * PBInv - .5;
					if (val.pb != pb)
					{
						val.pb = pb;
						const auto freq = val.getFreq(xen);
						setFrequencyHz(freq);
						applyFilter(samples, numChannels, s, ts);
						s = ts;
					}
				}
			}

			applyFilter(samples, numChannels, s, numSamples);
		}

		void Resonator::applyFilter(double** samples, int numChannels,
			int startIdx, int endIdx) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				for (auto i = startIdx; i < endIdx; ++i)
				{
					const auto dry = smpls[i];
					auto wet = 0.;
					for (auto f = 0; f < numFiltersBelowNyquist; ++f)
						wet += resonators[f](dry, ch) * material.getMag(f);
					smpls[i] = wet;
				}
			}
		}

		// Voice

		Voice::Voice(const arch::XenManager& xen, const DualMaterial& material) :
			filter(xen, material),
			env(),
			envBuffer(),
			gain(1.)
		{}

		void Voice::prepare(double sampleRate)
		{
			filter.prepare(sampleRate);
			env.prepare(sampleRate);
		}

		void Voice::updateParameters(double atk, double dcy, double sus, double rls) noexcept
		{
			env.updateParameters(atk, dcy, sus, rls);
		}

		void Voice::setBandwidth(double bw) noexcept
		{
			filter.setBandwidth(bw);
		}

		void Voice::operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
			double transposeSemi, int numChannels, int numSamples) noexcept
		{
			synthesizeEnvelope(midi, numSamples);
			processEnvelope(samplesSrc, samplesDest, numChannels, numSamples);
			filter(samplesDest, midi, transposeSemi, numChannels, numSamples);
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::multiply(samplesDest[ch], gain, numSamples);
		}

		void Voice::synthesizeEnvelope(const MidiBuffer& midi, int numSamples) noexcept
		{
			auto s = 0;
			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				const auto ts = it.samplePosition;
				if (msg.isNoteOn())
				{
					s = synthesizeEnvelope(s, ts);
					env.noteOn = true;
				}
				else if (msg.isNoteOff() || msg.isAllNotesOff())
				{
					s = synthesizeEnvelope(s, ts);
					env.noteOn = false;
				}
				else
					s = synthesizeEnvelope(s, ts);
			}

			synthesizeEnvelope(s, numSamples);
		}

		int Voice::synthesizeEnvelope(int s, int ts) noexcept
		{
			while (s < ts)
			{
				envBuffer[s] = env();
				++s;
			}
			return s;
		}

		void Voice::processEnvelope(const double** samplesSrc, double** samplesDest,
			int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto smplsSrc = samplesSrc[ch];
				auto smplsDest = samplesDest[ch];

				SIMD::multiply(smplsDest, smplsSrc, envBuffer.data(), numSamples);
			}
		}

		void Voice::detectSleepy(bool& sleepy, double** samplesDest,
			int numChannels, int numSamples) noexcept
		{
			if (env.state != EnvelopeGenerator::State::Release)
			{
				sleepy = false;
				return;
			}

			const bool bufferTooSmall = numSamples != BlockSize;
			if (bufferTooSmall)
				return;

			sleepy = bufferSilent(samplesDest, numChannels, numSamples);
			if (!sleepy)
				return;

			filter.reset();
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::clear(samplesDest[ch], numSamples);
		}

		bool Voice::bufferSilent(double* const* samplesDest, int numChannels, int numSamples) const noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samplesDest[ch];
				for (auto s = 0; s < numSamples; ++s)
				{
					auto smpl = std::abs(smpls[s]);
					if (smpl > 1e-4)
						return false;
				}
			}
			return true;
		}

		// Filter

		Filter::Filter(const arch::XenManager& xen) :
			autoGainReso(),
			material(),
			voices
			{
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material)
			},
			modalMix(-1.),
			modalHarmonize(-1.),
			modalSaturate(-1.),
			reso(-1.),
			sampleRateInv(1.)
		{
			initAutoGainReso();
		}

		void Filter::prepare(double sampleRate)
		{
			for (auto& voice : voices)
				voice.prepare(sampleRate);
			reso = -1.;
			sampleRateInv = 1. / sampleRate;
		}

		void Filter::updateParameters(double _modalMix, double _modalHarmonize,
			double _modalSaturate, double _reso) noexcept
		{
			updateModalMix(_modalMix, _modalHarmonize, _modalSaturate);
			updateReso(_reso);
		}

		void Filter::operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
			double transposeSemi, int numChannels, int numSamples, int v) noexcept
		{
			voices[v](samplesSrc, samplesDest, midi, transposeSemi, numChannels, numSamples);
		}

		void Filter::updateModalMix(double _modalMix, double _modalHarmonize, double _modalSaturate) noexcept
		{
			bool wantsUpdate = material.wantsUpdate();
			if (modalMix == _modalMix && modalHarmonize == _modalHarmonize && modalSaturate == _modalSaturate && !wantsUpdate)
				return;
			modalMix = _modalMix;
			modalHarmonize = _modalHarmonize;
			modalSaturate = _modalSaturate;
			material.setMix(modalMix, modalHarmonize, modalSaturate);
			for (auto& voice : voices)
				voice.filter.updateFreqRatios();
		}

		void Filter::updateReso(double _reso) noexcept
		{
			if (reso == _reso)
				return;
			reso = _reso;
			autoGainReso.updateParameterValue(reso);
			const auto bwFc = calcBandwidthFc(reso, sampleRateInv);
			for (auto& voice : voices)
			{
				voice.setBandwidth(bwFc);
				voice.gain = autoGainReso();
			}
		}

		void Filter::initAutoGainReso()
		{
			Resonator2 resonator;
			resonator.setCutoffFc(500. / 44100.);

			autoGainReso.prepareGains
			(
				[&](double* smpls, double _reso, int numSamples)
				{
					reso = _reso;
					const auto bwFc = calcBandwidthFc(reso, 1. / 44100.);
					resonator.setBandwidth(bwFc);
					resonator.update();
					resonator.reset();
					for (auto s = 0; s < numSamples; ++s)
					{
						const auto dry = smpls[s];
						const auto wet = resonator(dry);
						smpls[s] = wet;
					}
				}
			);
		}
	}
}