#pragma once
#include "../../../arch/XenManager.h"
#include "Material.h"
#include "../Resonator.h"

namespace dsp
{
	namespace modal2
	{
		inline double calcBandwidthFc(double reso, double sampleRateInv) noexcept
		{
			const auto bw = 20. - math::tanhApprox(3. * reso) * 19.;
			const auto bwFc = bw * sampleRateInv;
			return bwFc;
		}

		class ResonatorBank
		{
			using ResonatorArray = std::array<ResonatorStereo2, NumFilters>;

			struct Val
			{
				Val() :
					pitch(0.),
					transpose(0.),
					pb(0.),
					pbRange(2.),
					xen(12.)
				{}

				double getFreq(const arch::XenManager& _xen) noexcept
				{
					return _xen.noteToFreqHzWithWrap(pitch + transpose + pb * pbRange * 2.);
				}

				double pitch, transpose, pb, pbRange, xen;
			};

		public:
			ResonatorBank() :
				resonators(),
				val(),
				freqHz(1000.),
				sampleRate(1.),
				sampleRateInv(1.),
				nyquist(.5),
				gain(1.),
				numFiltersBelowNyquist(0)
			{
			}

			void reset() noexcept
			{
				for (auto i = 0; i < NumFilters; ++i)
					resonators[i].reset();
			}

			void prepare(const MaterialData& material, double _sampleRate)
			{
				sampleRate = _sampleRate;
				sampleRateInv = 1. / sampleRate;
				nyquist = sampleRate * .5;
				reset();
				setFrequencyHz(material, 1000.);
				setReso(.25);
			}

			// material, samples, midi, xen, reso, transposeSemi, numChannels, numSamples
			void operator()(const MaterialData& material, double** samples, const MidiBuffer& midi,
				const arch::XenManager& xen, double reso, double transposeSemi,
				int numChannels, int numSamples) noexcept
			{
				setReso(reso);
				process(material, samples, midi, xen, transposeSemi, numChannels, numSamples);
			}

			// freqHz
			void setFrequencyHz(const MaterialData& material, double freq) noexcept
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

			void updateFreqRatios(const MaterialData& material) noexcept
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

			// bw
			void setReso(double reso) noexcept
			{
				const auto bw = calcBandwidthFc(reso, sampleRateInv);
				gain = 1. + Tau * reso * reso;
				for (auto i = 0; i < NumFilters; ++i)
				{
					auto& resonator = resonators[i];
					resonator.setBandwidth(bw);
					resonator.update();
				}
			}

		private:
			ResonatorArray resonators;
			Val val;
			double freqHz, sampleRate, sampleRateInv, nyquist, gain;
			int numFiltersBelowNyquist;

			/* material, samples, midi, xen, numChannels, numSamples */
			void process(const MaterialData& material, double** samples, const MidiBuffer& midi,
				const arch::XenManager& xen, double transposeSemi,
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
						setFrequencyHz(material, freq);
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
						setFrequencyHz(material, freq);
						applyFilter(material, samples, numChannels, s, ts);
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
							setFrequencyHz(material, freq);
							applyFilter(material, samples, numChannels, s, ts);
							s = ts;
						}
					}
				}
				applyFilter(material, samples, numChannels, s, numSamples);
			}

			// material, samples, numChannels, startIdx, endIdx
			void applyFilter(const MaterialData& material, double** samples,
				int numChannels, int startIdx, int endIdx) noexcept
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samples[ch];
					for (auto i = startIdx; i < endIdx; ++i)
					{
						const auto dry = smpls[i];
						auto wet = 0.;
						for (auto f = 0; f < numFiltersBelowNyquist; ++f)
							wet += resonators[f](dry, ch) * material.getMag(f) * gain;
						smpls[i] = wet;
					}
				}
			}
		};
	}
}

/*

*/