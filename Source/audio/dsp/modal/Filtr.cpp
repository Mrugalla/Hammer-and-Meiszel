#include "Filtr.h"

namespace dsp
{
	namespace modal
	{
		double calcBandwidthFc(double reso, double sampleRateInv) noexcept
		{
			const auto bw = 20. - math::tanhApprox(3. * reso) * 19.;
			const auto bwFc = bw * sampleRateInv;
			return bwFc;
		}

		Filtr::Filtr(const arch::XenManager& xen) :
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

		void Filtr::prepare(double sampleRate)
		{
			for (auto& voice : voices)
				voice.prepare(sampleRate);
			reso = -1.;
			sampleRateInv = 1. / sampleRate;
		}

		void Filtr::updateParameters(double _modalMix, double _modalHarmonize,
			double _modalSaturate, double _reso) noexcept
		{
			const bool materialHasUpdated = material.hasUpdated();
			if (materialHasUpdated)
			{
				initAutoGainReso();
				material.setMix(modalMix, modalHarmonize, modalSaturate);
				for (auto& voice : voices)
					voice.filter.updateFreqRatios();
				autoGainReso.updateParameterValue(reso);
				const auto bwFc = calcBandwidthFc(reso, sampleRateInv);
				for (auto& voice : voices)
				{
					voice.setBandwidth(bwFc);
					voice.gain = autoGainReso();
				}
				for(auto i = 0; i < 2; ++i)
					if(material.materials[i].status.load() == Status::UpdatedMaterial)
						material.materials[i].status.store(Status::UpdatedDualMaterial);
			}
			updateModalMix(_modalMix, _modalHarmonize, _modalSaturate);
			updateReso(_reso);
		}

		void Filtr::operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
			double transposeSemi, int numChannels, int numSamples, int v) noexcept
		{
			voices[v](samplesSrc, samplesDest, midi, transposeSemi, numChannels, numSamples);
		}

		void Filtr::updateModalMix(double _modalMix, double _modalHarmonize, double _modalSaturate) noexcept
		{
			if (modalMix == _modalMix && modalHarmonize == _modalHarmonize && modalSaturate == _modalSaturate)
				return;
			modalMix = _modalMix;
			modalHarmonize = _modalHarmonize;
			modalSaturate = _modalSaturate;
			material.setMix(modalMix, modalHarmonize, modalSaturate);
			for (auto& voice : voices)
				voice.filter.updateFreqRatios();
		}

		void Filtr::updateReso(double _reso) noexcept
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

		void Filtr::initAutoGainReso()
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