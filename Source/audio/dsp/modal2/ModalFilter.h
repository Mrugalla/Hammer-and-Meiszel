#pragma once
#include "Voice.h"

namespace dsp
{
	namespace modal2
	{
		struct ModalFilter
		{
			ModalFilter() :
				materials(),
				envGenParams(),
				voices
				{
					Voice(envGenParams), Voice(envGenParams), Voice(envGenParams), Voice(envGenParams),
					Voice(envGenParams), Voice(envGenParams), Voice(envGenParams), Voice(envGenParams),
					Voice(envGenParams), Voice(envGenParams), Voice(envGenParams), Voice(envGenParams),
					Voice(envGenParams), Voice(envGenParams), Voice(envGenParams)
				}
			{
				generateSaw(materials[0]);
				generateSquare(materials[1]);
			}

			void prepare(double sampleRate) noexcept
			{
				envGenParams.prepare(sampleRate);
				for (auto v = 0; v < voices.size(); ++v)
				{
					auto& voice = voices[v];
					voice.prepare(sampleRate);
				}
			}

			void operator()(const EnvelopeGenerator::Parameters& _envGenParams) noexcept
			{
				envGenParams.updateParameters(_envGenParams);

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

			void operator()(const double** samplesSrc, double** samplesDest,
				const MidiBuffer& midi, const arch::XenManager& xen,
				const Voice::Parameters& voiceParams, double transposeSemi,
				int numChannels, int numSamples, int v) noexcept
			{
				auto& voice = voices[v];
				voice
				(
					materials, voiceParams,
					samplesSrc, samplesDest,
					midi, xen, transposeSemi,
					numChannels, numSamples
				);
			}

			bool isSleepy(bool& sleepy, double** samplesDest,
				int numChannels, int numSamples, int v) noexcept
			{
				return voices[v].isSleepy(sleepy, samplesDest, numChannels, numSamples);
			}

			Material& getMaterial(int i) noexcept
			{
				return materials[i];
			}

			const Material& getMaterial(int i) const noexcept
			{
				return materials[i];
			}

		private:
			DualMaterial materials;
			EnvelopeGenerator::Parameters envGenParams;
			std::array<Voice, NumMPEChannels> voices;
		};
	}
}