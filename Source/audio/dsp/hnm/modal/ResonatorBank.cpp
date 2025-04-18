#include "ResonatorBank.h"

namespace dsp
{
	namespace modal
	{
		ResonatorBank::Val::Val() :
			pitch(0.),
			transpose(0.),
			pb(0.)
		{}

		void ResonatorBank::Val::reset() noexcept
		{
			pitch = 0.;
			transpose = 0.;
			pb = 0.;
		}

		double ResonatorBank::Val::getFreq(const arch::XenManager& xen) noexcept
		{
			const auto pbRange = xen.getPitchbendRange();
			return xen.noteToFreqHz(pitch + transpose + pb * pbRange);
		}

		//

		ResonatorBank::ResoGain::ResoGain() :
			gains(),
			gain{ 1., 1. }
		{
			gains[0] = 1.;
			gains[1] = math::dbToAmp(24.);
			gains[2] = math::dbToAmp(32.);
			gains[NumGains] = gains[NumGains - 1];
		}

		void ResonatorBank::ResoGain::update(double x, int ch) noexcept
		{
			x *= MaxGainD;
			const auto xFloor = std::floor(x);
			const auto xFrac = x - xFloor;
			const auto i0 = static_cast<int>(xFloor);
			const auto i1 = i0 + 1;
			const auto g0 = gains[i0];
			const auto g1 = gains[i1];
			const auto gR = g1 - g0;
			gain[ch] = g0 + xFrac * gR;
		}

		double ResonatorBank::ResoGain::operator()(int ch) const noexcept
		{
			return gain[ch];
		}

		//

		ResonatorBank::ResonatorBank() :
			resonators(),
			val(),
			freqHz(1000.),
			sampleRate(1.),
			sampleRateInv(1.),
			nyquist(.5),
			autoGainReso(),
			numFiltersBelowNyquist{ 0, 0 },
			sleepy()
		{
		}

		void ResonatorBank::reset() noexcept
		{
			for(auto ch = 0; ch < 2; ++ch)
				for (auto i = 0; i < NumPartials; ++i)
					resonators[i].reset(ch);
		}

		void ResonatorBank::prepare(const MaterialDataStereo& materialStereo, double _sampleRate)
		{
			sampleRate = _sampleRate;
			sampleRateInv = 1. / sampleRate;
			nyquist = sampleRate * .5;
			reset();
			val.reset();
			for (auto& n : numFiltersBelowNyquist)
				n = 0;
			setFrequencyHz(materialStereo, 1000., 2);
			for(auto ch = 0; ch < 2; ++ch)
				setReso(.25, ch);
			sleepy.prepare(sampleRate);
		}

		void ResonatorBank::operator()(const MaterialDataStereo& materialStereo,
			double** samples, int numChannels, int numSamples) noexcept
		{
			applyFilter(materialStereo, samples, numChannels, numSamples);
			sleepy(samples, numChannels, numSamples);
		}

		void ResonatorBank::setTranspose(const MaterialDataStereo& materialStereo,
			const arch::XenManager& xen, double transposeSemi, int numChannels) noexcept
		{
			val.transpose = transposeSemi;
			const auto freq = val.getFreq(xen);
			setFrequencyHz(materialStereo, freq, numChannels);
		}

		void ResonatorBank::triggerXen(const arch::XenManager& xen,
			const MaterialDataStereo& materialStereo, int numChannels) noexcept
		{
			const auto freq = val.getFreq(xen);
			setFrequencyHz(materialStereo, freq, numChannels);
		}

		void ResonatorBank::triggerNoteOn(const MaterialDataStereo& materialStereo,
			const arch::XenManager& xen, double noteNumber, int numChannels, bool polyphonic) noexcept
		{
			val.pitch = noteNumber;
			const auto freq = val.getFreq(xen);
			if (setFrequencyHz(materialStereo, freq, numChannels))
				if(polyphonic)
					reset();
			sleepy.triggerNoteOn();
		}

		void ResonatorBank::triggerNoteOff() noexcept
		{
			sleepy.triggerNoteOff();
		}

		void ResonatorBank::triggerPitchbend(const MaterialDataStereo& materialStereo, const arch::XenManager& xen,
			double pitchbend, int numChannels) noexcept
		{
			val.pb = pitchbend;
			const auto freq = val.getFreq(xen);
			setFrequencyHz(materialStereo, freq, numChannels);
		}

		bool ResonatorBank::isRinging() const noexcept
		{
			return sleepy.isRinging();
		}

		bool ResonatorBank::setFrequencyHz(const MaterialDataStereo& materialStereo,
			double freq, int numChannels) noexcept
		{
			const auto freqSafe = math::limit(MinFreqHz, nyquist, freq);
			if (freqHz == freqSafe)
				return false;
			freqHz = freqSafe;
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto& material = materialStereo[ch];
				auto& nfbn = numFiltersBelowNyquist[ch];
				updateFreqRatios(material, nfbn, ch);
			}
			return true;
		}

		void ResonatorBank::updateFreqRatios(const MaterialDataStereo& materialStereo, int numChannels) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				updateFreqRatios(materialStereo[ch], numFiltersBelowNyquist[ch], ch);
		}

		void ResonatorBank::setReso(double reso, int ch) noexcept
		{
			static constexpr auto BWStart = 200.;
			static constexpr auto BWEnd = 1.;
			static constexpr auto BWRange = BWEnd - BWStart;

			const auto resoSqrt = std::sqrt(reso);
			const auto resoScaled = resoSqrt * 3.;
			const auto resoMapped = math::tanhApprox(resoScaled);
			const auto bw = (BWStart + resoMapped * BWRange) * sampleRateInv;
			autoGainReso.update(reso, ch);
			for (auto i = 0; i < NumPartials; ++i)
			{
				auto& resonator = resonators[i];
				resonator.setBandwidth(bw, ch);
				resonator.update(ch);
			}
		}

		void ResonatorBank::updateFreqRatios(const MaterialData& material, int& nfbn, int ch) noexcept
		{
			nfbn = 0;
			for (auto i = 0; i < NumPartials; ++i)
			{
				const auto pFc = material.getFc(i);
				const auto fcKeytracked = freqHz * pFc;
				if (fcKeytracked < nyquist)
				{
					const auto fc = math::freqHzToFc(fcKeytracked, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc, ch);
					resonator.update(ch);
					nfbn = i + 1;
				}
				else return;
			}
		}

		void ResonatorBank::applyFilter(const MaterialDataStereo& materialStereo, double** samples,
			int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto& material = materialStereo[ch];
				const auto nfbn = numFiltersBelowNyquist[ch];
				const auto autoGain = autoGainReso(ch);
				auto smpls = samples[ch];

				for (auto i = 0; i < numSamples; ++i)
				{
					const auto dry = smpls[i];
					auto wet = 0.;
					
					for (auto f = 0; f < nfbn; ++f)
					{
						auto& resonator = resonators[f];
						const auto mag = material.getMag(f);
						const auto bpY = resonator(dry, ch) * mag;
						wet += bpY;
					}

					wet *= autoGain;
					smpls[i] = wet;
				}
			}
		}
	}
}
