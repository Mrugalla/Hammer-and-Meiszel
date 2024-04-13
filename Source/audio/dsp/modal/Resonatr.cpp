#include "Resonatr.h"

namespace dsp
{
	namespace modal
	{
		Resonatr::Val::Val() :
			pitch(0.),
			transpose(0.),
			pb(0.),
			pbRange(2.),
			xen(12.)
		{}

		double Resonatr::Val::getFreq(const arch::XenManager& _xen) noexcept
		{
			return _xen.noteToFreqHzWithWrap(pitch + transpose + pb * pbRange * 2.);
		}

		Resonatr::Resonatr(const arch::XenManager& _xen, const DualMaterial& _material) :
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

		void Resonatr::reset() noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
				resonators[i].reset();
		}

		void Resonatr::prepare(double _sampleRate)
		{
			sampleRate = _sampleRate;
			nyquist = sampleRate * .5;
			setFrequencyHz(1000.);
		}

		void Resonatr::operator()(double** samples, const MidiBuffer& midi,
			int numChannels, int numSamples) noexcept
		{
			process(samples, midi, numChannels, numSamples);
		}

		void Resonatr::setFrequencyHz(double freq) noexcept
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

		void Resonatr::updateFreqRatios() noexcept
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

		void Resonatr::setBandwidth(double bw) noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				auto& resonator = resonators[i];
				resonator.setBandwidth(bw);
				resonator.update();
			}
		}

		void Resonatr::process(double** samples, const MidiBuffer& midi, int numChannels, int numSamples) noexcept
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

		void Resonatr::applyFilter(double** samples, int numChannels,
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
	}
}