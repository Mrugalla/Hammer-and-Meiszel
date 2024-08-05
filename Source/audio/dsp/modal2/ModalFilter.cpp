#include "ModalFilter.h"

namespace dsp
{
	namespace modal2
	{
		ModalFilter::ModalFilter() :
			materials(),
			voices()
		{
			generateSaw(materials[0]);
			generateSquare(materials[1]);
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
			for (auto m = 0; m < 2; ++m)
			{
				auto& material = materials[m];
				auto& status = material.status;
				if (status == Status::UpdatedMaterial)
				{
					for (auto& voice : voices)
						voice.reportMaterialUpdate();
					status = Status::UpdatedProcessor;
				}
			}
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

		Material& ModalFilter::getMaterial(int i) noexcept
		{
			return materials[i];
		}

		const Material& ModalFilter::getMaterial(int i) const noexcept
		{
			return materials[i];
		}
	}
}