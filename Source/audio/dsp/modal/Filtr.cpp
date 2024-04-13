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

		Filtr::Filtr(const arch::XenManager& _xen) :
			xen(_xen),
			autoGainReso(),
			material(),
			voices
			{
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material)
			},
			sampleRateInv(1.),
			pbRangeP(2.),
			xenP(12.),
			blendPRM(0.),
			spreizungPRM(0.),
			harmoniePRM(0.),
			tonalitaetPRM(0.),
			resoPRM(0.),
			transposeSemiPRM(0.)
		{
			initAutoGainReso();
		}

		void Filtr::prepare(double sampleRate)
		{
			for (auto& voice : voices)
				voice.prepare(sampleRate);
			sampleRateInv = 1. / sampleRate;

			const auto smoothLenMs = 4.;
			blendPRM.prepare(sampleRate, smoothLenMs);
			spreizungPRM.prepare(sampleRate, smoothLenMs);
			harmoniePRM.prepare(sampleRate, smoothLenMs);
			tonalitaetPRM.prepare(sampleRate, smoothLenMs);
			resoPRM.prepare(sampleRate, smoothLenMs);
			transposeSemiPRM.prepare(sampleRate, smoothLenMs);
		}

		void Filtr::updateParameters(double blend, double spreizung, double harmonie,
			double tonalitaet, double reso, double transposeSemi) noexcept
		{
			const bool materialHasUpdated = material.hasUpdated();
			if (materialHasUpdated)
			{
				initAutoGainReso();
				material.setMix(blendPRM, spreizungPRM, harmoniePRM, tonalitaetPRM);
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
			updateModalMix(blend, spreizung, harmonie, tonalitaet);
			updateReso(reso);

			const auto transposeSemiInfo = transposeSemiPRM(transposeSemi);
			if (transposeSemiInfo.smoothing || pbRangeP != xen.getPitchbendRange() || xenP != xen.getXen())
			{
				pbRangeP = xen.getPitchbendRange();
				xenP = xen.getXen();
				for (auto& voice : voices)
					voice.updateResonatorVal(transposeSemi, pbRangeP, xenP);
			}
		}

		void Filtr::operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
			int numChannels, int numSamples, int v) noexcept
		{
			voices[v](samplesSrc, samplesDest, midi, numChannels, numSamples);
		}

		void Filtr::updateModalMix(double blend, double spreizung, double harmonie, double tonalitaet) noexcept
		{
			const auto blendInfo = blendPRM(blend);
			const auto spreizungInfo = spreizungPRM(spreizung);
			const auto harmonieInfo = harmoniePRM(harmonie);
			const auto tonalitaetInfo = tonalitaetPRM(tonalitaet);

			if (blendInfo.smoothing || spreizungInfo.smoothing || harmonieInfo.smoothing || tonalitaetInfo.smoothing)
			{
				material.setMix(blendPRM, spreizungPRM, harmoniePRM, tonalitaetPRM);
				for (auto& voice : voices)
					voice.filter.updateFreqRatios();
			}
		}

		void Filtr::updateReso(double reso) noexcept
		{
			auto resoInfo = resoPRM(reso);
			if (!resoInfo.smoothing)
				return;
			autoGainReso.updateParameterValue(resoPRM);
			const auto bwFc = calcBandwidthFc(resoPRM, sampleRateInv);
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
				[&](double* smpls, double reso, int numSamples)
				{
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