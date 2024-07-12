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
			freqHz(1000.),
			sampleRate(1.),
			sampleRateInv(1.),
			nyquist(.5),
			gain(1.),
			numFiltersBelowNyquist{ 0, 0 }
		{
		}

		void ResonatorBank::reset() noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
				resonators[i].reset();
		}

		void ResonatorBank::prepare(const MaterialDataStereo& materialStereo, double _sampleRate)
		{
			sampleRate = _sampleRate;
			sampleRateInv = 1. / sampleRate;
			nyquist = sampleRate * .5;
			reset();
			setFrequencyHz(materialStereo, 1000., 2);
			setReso(.25, 2);
		}

		void ResonatorBank::operator()(const MaterialDataStereo& materialStereo, double** samples, const MidiBuffer& midi,
			const arch::XenManager& xen, double reso, double transposeSemi,
			int numChannels, int numSamples) noexcept
		{
			setReso(reso, numChannels);
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
				numFiltersBelowNyquist[ch] = NumFilters;
				for (auto i = 0; i < NumFilters; ++i)
				{
					const auto freqRatio = material.getRatio(i);
					const auto freqFilter = freqHz * freqRatio;
					if (freqFilter < nyquist)
					{
						const auto fc = math::freqHzToFc(freqFilter, sampleRate);
						auto& resonator = resonators[i];
						resonator.setCutoffFc(fc, ch);
						resonator.update(ch);
					}
					else
					{
						numFiltersBelowNyquist[ch] = i;
						i = NumFilters;
					}
				}
			}
		}

		void ResonatorBank::updateFreqRatios(const MaterialDataStereo& materialStereo, int numChannels) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				updateFreqRatios(materialStereo[ch], numFiltersBelowNyquist[ch], ch);
		}

		void ResonatorBank::setReso(double reso, int numChannels) noexcept
		{
			const auto bw = calcBandwidthFc(reso, sampleRateInv);
			gain = 1. + Tau * reso * reso;
			for(auto ch = 0; ch < numChannels; ++ch)
				for (auto i = 0; i < NumFilters; ++i)
				{
					auto& resonator = resonators[i];
					resonator.setBandwidth(bw, ch);
					resonator.update(ch);
				}
		}

		void ResonatorBank::updateFreqRatios(const MaterialData& material, int& nfbn, int ch) noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto freqRatio = material.getRatio(i);
				const auto freqFilter = freqHz * freqRatio;
				if (freqFilter < nyquist)
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
			static constexpr double PB = 0x3fff;
			static constexpr double PBInv = 1. / PB;

			{
				const auto pbRange = xen.getPitchbendRange(); // can be improved by being triggered from xenManger changes directly
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
					const auto ts = it.samplePosition;
					val.pitch = static_cast<double>(msg.getNoteNumber());
					const auto freq = val.getFreq(xen);
					setFrequencyHz(materialStereo, freq, numChannels);
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
					smpls[i] = wet;
				}
			}
		}
	}
}
