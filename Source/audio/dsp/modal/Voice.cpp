#include "Voice.h"

namespace dsp
{
    namespace modal
    {
		Voice::Voice(const arch::XenManager& xen, const EnvelopeGenerator::Parameters& envGenParams) :
			peaks(),
			filter(xen, peaks),
			env(envGenParams),
			envBuffer(),
			gain(1.)
		{
		}

		void Voice::prepare(double sampleRate)
		{
			filter.prepare(sampleRate);
			env.prepare(sampleRate);
		}

		void Voice::setBandwidth(double bw) noexcept
		{
			filter.setBandwidth(bw);
		}

		void Voice::operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
			int numChannels, int numSamples) noexcept
		{
			synthesizeEnvelope(midi, numSamples);
			processEnvelope(samplesSrc, samplesDest, numChannels, numSamples);
			filter(samplesDest, midi, numChannels, numSamples);
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::multiply(samplesDest[ch], gain, numSamples);
		}

		void Voice::synthesizeEnvelope(const MidiBuffer& midi, int numSamples) noexcept
		{
			auto s = 0;
			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				const auto ts = it.samplePosition;
				if (msg.isNoteOn())
				{
					s = synthesizeEnvelope(s, ts);
					env.noteOn = true;
				}
				else if (msg.isNoteOff() || msg.isAllNotesOff())
				{
					s = synthesizeEnvelope(s, ts);
					env.noteOn = false;
				}
				else
					s = synthesizeEnvelope(s, ts);
			}

			synthesizeEnvelope(s, numSamples);
		}

		int Voice::synthesizeEnvelope(int s, int ts) noexcept
		{
			while (s < ts)
			{
				envBuffer[s] = env();
				++s;
			}
			return s;
		}

		void Voice::processEnvelope(const double** samplesSrc, double** samplesDest,
			int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto smplsSrc = samplesSrc[ch];
				auto smplsDest = samplesDest[ch];

				SIMD::multiply(smplsDest, smplsSrc, envBuffer.data(), numSamples);
			}
		}

		void Voice::detectSleepy(bool& sleepy, double** samplesDest,
			int numChannels, int numSamples) noexcept
		{
			if (env.state != EnvelopeGenerator::State::Release)
			{
				sleepy = false;
				return;
			}

			const bool bufferTooSmall = numSamples != BlockSize;
			if (bufferTooSmall)
				return;

			sleepy = bufferSilent(samplesDest, numChannels, numSamples);
			if (!sleepy)
				return;

			filter.reset();
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::clear(samplesDest[ch], numSamples);
		}

		bool Voice::bufferSilent(double* const* samplesDest, int numChannels, int numSamples) const noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samplesDest[ch];
				for (auto s = 0; s < numSamples; ++s)
				{
					auto smpl = std::abs(smpls[s]);
					if (smpl > 1e-4)
						return false;
				}
			}
			return true;
		}
    }
}