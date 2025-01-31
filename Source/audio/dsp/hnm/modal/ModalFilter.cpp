#include "ModalFilter.h"

namespace dsp
{
	namespace modal
	{
		ModalFilter::ModalFilter() :
			materials(),
			voices()
		{
		}

		void ModalFilter::prepare(double sampleRate) noexcept
		{
			materials.reportUpdate();
			for (auto v = 0; v < voices.size(); ++v)
			{
				auto& voice = voices[v];
				voice.prepare(sampleRate);
			}
		}

		void ModalFilter::operator()() noexcept
		{
			if (!materials.updated())
				return;

			for (auto& voice : voices)
				voice.reportMaterialUpdate();
			materials.reportUpdate();
		}

		void ModalFilter::operator()(double** samples,
			const MidiBuffer& midi, const arch::XenManager& xen,
			const Voice::Parameters& voiceParams,
			double transposeSemi, double envGenMod,
			int numChannels, int numSamples, int v) noexcept
		{
			auto& voice = voices[v];
			voice
			(
				materials, voiceParams,
				samples, midi, xen,
				transposeSemi, envGenMod,
				numChannels, numSamples
			);
		}

		bool ModalFilter::isRinging(int i) const noexcept
		{
			return voices[i].isRinging();
		}

		Material& ModalFilter::getMaterial(int i) noexcept
		{
			return materials.getMaterial(i);
		}

		const Material& ModalFilter::getMaterial(int i) const noexcept
		{
			return materials.getMaterial(i);
		}

		ActivesArray& ModalFilter::getActives() noexcept
		{
			return materials.getActives();
		}

		void ModalFilter::setSolo(bool e) noexcept
		{
			materials.getMaterial(0).soloing = e;
			materials.getMaterial(1).soloing = e;
		}

		void ModalFilter::randomizeMaterial(arch::RandSeed& rand, const arch::XenManager& xen, int mIdx)
		{
			auto& mat = materials.getMaterial(mIdx);
			if (mat.status != dsp::modal::StatusMat::Processing)
				return;
			std::array<double, dsp::modal::NumFilters> ratios;
			ratios[0] = 1.;
			for (auto j = 1; j < dsp::modal::NumFilters; ++j)
				ratios[j] = 1. + static_cast<double>(rand()) * 32.;
			std::sort(ratios.begin(), ratios.end());

			auto& peaks = mat.peakInfos;

			peaks[0].mag = static_cast<double>(rand());
			peaks[0].ratio = ratios[0];
			peaks[0].freqHz = xen.noteToFreqHz(static_cast<double>(rand()) * 127.);
			peaks[0].keytrack = 0.;

			for (auto j = 1; j < dsp::modal::NumFilters; ++j)
			{
				auto& peak = peaks[j];
				peak.mag = static_cast<double>(rand());
				peak.ratio = ratios[j];
				peak.freqHz = xen.noteToFreqHz(static_cast<double>(rand()) * 127.);
				peak.keytrack = static_cast<double>(rand());
			}

			mat.reportUpdate();
		}
	}
}