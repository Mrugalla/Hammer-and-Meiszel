#include "ModalFilter.h"

namespace dsp
{
	namespace modal
	{
		ModalFilter::ModalFilter() :
			materials(),
			voices(),
			transposeSemi(0.)
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

		void ModalFilter::operator()(double** samples, const Voice::Parameters& params,
			double envGenMod, int numChannels, int numSamples, int v) noexcept
		{
			voices[v]
			(
				samples, materials, params,
				envGenMod, numChannels, numSamples
			);
		}

		void ModalFilter::setTranspose(const arch::XenManager& xen, double t, int numChannels) noexcept
		{
			if (transposeSemi == t)
				return;
			transposeSemi = t;
			for (auto& voice : voices)
				voice.setTranspose(xen, transposeSemi, numChannels);
		}

		void ModalFilter::triggerXen(const arch::XenManager& xen,
			int numChannels) noexcept
		{
			for(auto& voice: voices)
				voice.triggerXen(xen, numChannels);
		}

		void ModalFilter::triggerNoteOn(const arch::XenManager& xen,
			double noteNumber, int numChannels, int v) noexcept
		{
			voices[v].triggerNoteOn(xen, noteNumber, numChannels);
		}

		void ModalFilter::triggerNoteOff(int v) noexcept
		{
			voices[v].triggerNoteOff();
		}

		void ModalFilter::triggerPitchbend(const arch::XenManager& xen,
			double pitchbend, int numChannels, int v) noexcept
		{
			voices[v].triggerPitchbend(xen, pitchbend, numChannels);
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

		void ModalFilter::randomizeMaterial(arch::RandSeed& rand, int mIdx)
		{
			auto& mat = materials.getMaterial(mIdx);
			if (mat.status != dsp::modal::StatusMat::Processing)
				return;
			auto& peaks = mat.peakInfos;
			peaks[0].fc = 1.;
			peaks[0].mag = static_cast<double>(rand());
			for (auto j = 1; j < NumPartials; ++j)
			{
				auto& peak = peaks[j];
				peak.mag = static_cast<double>(rand());
				peak.fc = 1. + static_cast<double>(rand()) * 32.;
			}
			mat.reportEndGesture();
		}
	}
}