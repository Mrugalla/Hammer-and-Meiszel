#include "ResonatorBank.h"

namespace dsp
{
	namespace modal
	{
		double calcBandwidthFc(double reso, double sampleRateInv) noexcept
		{
			static constexpr auto Min = 20.;
			static constexpr auto Range = 19.;

			const auto remap = math::tanhApprox(3. * reso);
			const auto bw = Min - remap * Range;
			const auto bwFc = bw * sampleRateInv;
			return bwFc;
		}

		ResonatorBank::Val::Val() :
			pitch(0.),
			transpose(0.),
			pb(0.),
			pbRange(2.),
			xen(0.),
			masterTune(0.),
			anchor(0.)
		{}

		void ResonatorBank::Val::reset() noexcept
		{
			pitch = 0.;
			transpose = 0.;
			pb = 0.;
			pbRange = 2.;
			xen = 0.;
		}

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
			ringing(false)
		{
		}

		void ResonatorBank::reset(int numChannels) noexcept
		{
			for(auto ch = 0; ch < numChannels; ++ch)
				for (auto i = 0; i < NumPartials; ++i)
					resonators[i].reset(ch);
			for (auto& n : numFiltersBelowNyquist)
				n = 0;
			sleepyTimer = 0;
			val.reset();
			ringing = false;
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
			sleepyTimerThreshold = static_cast<int>(nyquist / 8.);
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

		bool ResonatorBank::isRinging() const noexcept
		{
			return ringing;
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
				for (auto i = 0; i < NumPartials; ++i)
				{
					auto& resonator = resonators[i];
					resonator.setBandwidth(bw, ch);
					resonator.update(ch);
				}
				return;
			}

			static constexpr double NumPartialsInv = 1. / static_cast<double>(NumPartials);
			for (auto i = 0; i < NumPartialsInv; ++i)
			{
				const auto iF = static_cast<double>(i);
				const auto iR = iF * NumPartialsInv;
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
			for (auto i = 0; i < NumPartialsKeytracked; ++i)
			{
				const auto pFc = material.getFc(i);
				const auto fcKeytracked = freqHz * pFc;
				if (fcKeytracked < freqMax)
				{
					const auto fc = math::freqHzToFc(fcKeytracked, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc, ch);
					resonator.update(ch);
					nfbn = i + 1;
				}
				else
					break;
			}
			
			for (auto i = NumPartialsKeytracked; i < NumPartials; ++i)
			{
				const auto pFc = material.getFc(i);
				if (pFc < freqMax)
				{
					const auto fc = math::freqHzToFc(pFc, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc, ch);
					resonator.update(ch);
				}
			}
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
				const auto masterTune = xen.getMasterTune();
				const auto anchorPitch = xen.getAnchor();
				if (val.transpose != transposeSemi ||
					val.pbRange != pbRange ||
					val.xen != xenScale ||
					val.masterTune != masterTune ||
					val.anchor != anchorPitch)
				{
					val.transpose = transposeSemi;
					val.pbRange = pbRange;
					val.xen = xenScale;
					val.masterTune = masterTune;
					val.anchor = anchorPitch;
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
						applyFilter(materialStereo, samples, numChannels, s, ts);
						s = ts;
					}
				}
			}

			applyFilter(materialStereo, samples, numChannels, s, numSamples);
		}

		void ResonatorBank::applyFilter(const MaterialDataStereo& materialStereo, double** samples,
			int numChannels, int startIdx, int endIdx) noexcept
		{
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
						const auto mag = material.getMag(f);
						const auto bpY = resonator(dry, ch) * mag;
						wet += bpY;
					}

					for (auto f = NumPartialsKeytracked; f < NumPartials; ++f)
					{
						auto& resonator = resonators[f];
						const auto mag = material.getMag(f);
						//if (mag != 0.)
						{
							const auto bpY = resonator(dry, ch) * mag;
							wet += bpY;
						}
					}
					wet *= gain;

					if (wet * wet > 4.)
					{
						for (auto f = 0; f < nfbn; ++f)
							resonators[f].reset(ch);
						wet = 0.;
					}
					
					static constexpr auto Eps = 1e-7;
					if (wet * wet > Eps)
					{
						sleepyTimer = 0;
						ringing = true;
					}

					smpls[i] = wet;
				}
			}

			const auto blockLength = endIdx - startIdx;
			sleepyTimer += blockLength;
			ringing = sleepyTimer < sleepyTimerThreshold;
		}
	}
}
