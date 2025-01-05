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
	}
}