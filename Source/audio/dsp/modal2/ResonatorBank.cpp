#include "ResonatorBank.h"

namespace dsp
{
	namespace modal2
	{
		double calcBandwidthFc(double reso, double sampleRateInv) noexcept
		{
			const auto bw = 20. - math::tanhApprox(3. * reso) * 19.;
			const auto bwFc = bw * sampleRateInv;
			return bwFc;
		}

		ResonatorBank::Val::Val() :
			pitch(0.),
			transpose(0.),
			pb(0.),
			pbRange(2.),
			xen(12.)
		{}

		double ResonatorBank::Val::getFreq(const arch::XenManager& _xen) noexcept
		{
			return _xen.noteToFreqHzWithWrap(pitch + transpose + pb * pbRange * 2.);
		}

		ResonatorBank::ResonatorBank() :
			resonators(),
			val(),
			FreqMin(math::noteToFreqHz2(MinPitch)),
			freqMax(1.),
			freqHz(1000.),
			sampleRate(1.),
			sampleRateInv(1.),
			nyquist(.5),
			gains{ 1., 1. },
			numFiltersBelowNyquist{ 0, 0 },
			sleepyTimer(0),
			sleepyTimerThreshold(0),
			active(false)
		{
		}

		void ResonatorBank::reset(int numChannels) noexcept
		{
			for(auto ch = 0; ch < numChannels; ++ch)
				for (auto i = 0; i < NumFilters; ++i)
					resonators[i].reset(ch);
			for (auto& n : numFiltersBelowNyquist)
				n = 0;
			sleepyTimer = 0;
			active = false;
		}

		void ResonatorBank::prepare(const MaterialDataStereo& materialStereo, double _sampleRate)
		{
			sampleRate = _sampleRate;
			sampleRateInv = 1. / sampleRate;
			nyquist = sampleRate * .5;
			freqMax = math::noteToFreqHz2(math::freqHzToNote2(nyquist) - 1.);
			reset(2);
			setFrequencyHz(materialStereo, 1000., 2);
			for(auto ch = 0; ch < 2; ++ch)
				setReso(.25, 1., ch);
			sleepyTimerThreshold = static_cast<int>(nyquist) / 2;
		}

		void ResonatorBank::operator()(const MaterialDataStereo& materialStereo,
			double** samples, const MidiBuffer& midi,
			const arch::XenManager& xen, double transposeSemi,
			int numChannels, int numSamples) noexcept
		{
			process
			(
				materialStereo, samples, midi, xen,
				transposeSemi, numChannels, numSamples
			);
		}

		void ResonatorBank::setFrequencyHz(const MaterialDataStereo& materialStereo,
			double freq, int numChannels) noexcept
		{
			if (freqHz == freq)
				return;
			freqHz = freq;
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto& material = materialStereo[ch];
				auto& nfbn = numFiltersBelowNyquist[ch];
				updateFreqRatios(material, nfbn, ch);
			}
		}

		void ResonatorBank::updateFreqRatios(const MaterialDataStereo& materialStereo, int numChannels) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				updateFreqRatios(materialStereo[ch], numFiltersBelowNyquist[ch], ch);
		}

		void ResonatorBank::setReso(double reso, double damp, int ch) noexcept
		{
			gains[ch] = 1. + Tau * reso;

			if (damp == 0.)
			{
				const auto bw = calcBandwidthFc(reso, sampleRateInv);
				for (auto i = 0; i < NumFilters; ++i)
				{
					auto& resonator = resonators[i];
					resonator.setBandwidth(bw, ch);
					resonator.update(ch);
				}
				return;
			}

			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto iF = static_cast<double>(i);
				const auto iR = iF * NumFiltersInv;
				const auto iX = .5 * math::cosApprox(iR * Pi) + .5;
				const auto iM = 1. + damp * (iX - 1.);
				const auto resoR = reso * iM;
				const auto bw = calcBandwidthFc(resoR, sampleRateInv);

				auto& resonator = resonators[i];
				resonator.setBandwidth(bw, ch);
				resonator.update(ch);
			}
		}

		void ResonatorBank::updateFreqRatios(const MaterialData& material, int& nfbn, int ch) noexcept
		{
			nfbn = 0;
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto pRatio = material.getRatio(i);
				const auto pFreqHz = material.getFreqHz(i);
				const auto pKeytrack = material.getKeytrack(i);

				const auto freqKeytracked = freqHz * pRatio;

				const auto freqFilter = pFreqHz + pKeytrack * (freqKeytracked - pFreqHz);
				if (freqFilter < freqMax)
				{
					const auto fc = math::freqHzToFc(freqFilter, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc, ch);
					resonator.update(ch);
				}
				else
				{
					nfbn = i;
					return;
				}
			}
			nfbn = NumFilters;
		}

		void ResonatorBank::process(const MaterialDataStereo& materialStereo, double** samples,
			const MidiBuffer& midi, const arch::XenManager& xen, double transposeSemi,
			int numChannels, int numSamples) noexcept
		{
			static constexpr auto PB = static_cast<double>(0x3fff);
			static constexpr auto PBInv = 1. / PB;

			{
				const auto pbRange = xen.getPitchbendRange(); // can be improved by being triggered from xenManager changes directly
				const auto xenScale = xen.getXen();
				if (val.transpose != transposeSemi ||
					val.pbRange != pbRange ||
					val.xen != xenScale)
				{
					val.transpose = transposeSemi;
					val.pbRange = pbRange;
					val.xen = xenScale;
					const auto freq = val.getFreq(xen);
					setFrequencyHz(materialStereo, freq, numChannels);
				}
			}

			auto s = 0;
			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					if (!active)
						for (auto ch = 0; ch < numChannels; ++ch)
							for (auto i = 0; i < NumFilters; ++i)
								resonators[i].reset(ch);
					active = true;

					const auto ts = it.samplePosition;
					val.pitch = static_cast<double>(msg.getNoteNumber());
					const auto freq = val.getFreq(xen);
					setFrequencyHz(materialStereo, freq, numChannels);
					sleepyTimer = 0;
					applyFilter(materialStereo, samples, numChannels, s, ts);
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
						setFrequencyHz(materialStereo, freq, numChannels);
						if(active)
							applyFilter(materialStereo, samples, numChannels, s, ts);
						s = ts;
					}
				}
			}

			if(active)
				applyFilter(materialStereo, samples, numChannels, s, numSamples);
		}

		void ResonatorBank::applyFilter(const MaterialDataStereo& materialStereo, double** samples,
			int numChannels, int startIdx, int endIdx) noexcept
		{
			static constexpr auto Eps = 1e-6;

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto& material = materialStereo[ch];
				const auto nfbn = numFiltersBelowNyquist[ch];
				const auto gain = gains[ch];
				auto smpls = samples[ch];

				for (auto i = startIdx; i < endIdx; ++i)
				{
					const auto dry = smpls[i];
					auto wet = 0.;
					for (auto f = 0; f < nfbn; ++f)
					{
						auto& resonator = resonators[f];
						const auto bpY = resonator(dry, ch) * material.getMag(f) * gain;
						wet += bpY;
					}
					if (wet * wet > Eps)
						sleepyTimer = 0;
					smpls[i] = wet;
				}
			}

			const auto blockLength = endIdx - startIdx;
			sleepyTimer += blockLength;
			active = sleepyTimer < sleepyTimerThreshold;
		}
	}
}
