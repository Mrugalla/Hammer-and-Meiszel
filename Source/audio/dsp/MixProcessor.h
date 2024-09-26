#pragma once
#include "ParallelProcessor.h"
#include "Gain.h"
#include "WHead.h"

namespace dsp
{	
	struct LatencyCompensation
	{
		using DryBuffers = std::array<std::array<double, BlockSize>, 2>;

		LatencyCompensation() :
			ring(),
			wHead(),
			latency(0)
		{}

		void prepare(int _latency)
		{
			latency = _latency;
			if (latency != 0)
			{
				ring.setSize(2, latency, false, true, false);
				wHead.prepare(latency);
			}
			else
			{
				ring.setSize(0, 0);
				wHead.prepare(0);
			}
		}

		void operator()(DryBuffers& dryBuffers, const double* const* inputSamples,
			int numChannels, int numSamples) noexcept
		{
			if (latency != 0)
			{
				wHead(numSamples);

				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto smpls = inputSamples[ch];

					auto rng = ring.getWritePointer(ch);
					auto dry = dryBuffers[ch].data();

					for (auto s = 0; s < numSamples; ++s)
					{
						const auto w = wHead[s];
						const auto r = (w + 1) % latency;

						rng[w] = smpls[s];
						dry[s] = rng[r];
					}
				}
			}
			else
				for (auto ch = 0; ch < numChannels; ++ch)
					SIMD::copy(dryBuffers[ch].data(), inputSamples[ch], numSamples);
		}

		void operator()(double* const* samples, int numChannels, int numSamples) noexcept
		{
			if (latency != 0)
			{
				wHead(numSamples);

				for (auto ch = 0; ch < numChannels; ++ch)
				{
					const auto smpls = samples[ch];
					auto rng = ring.getWritePointer(ch);

					for (auto s = 0; s < numSamples; ++s)
					{
						const auto w = wHead[s];
						const auto r = (w + 1) % latency;

						rng[w] = smpls[s];
						smpls[s] = rng[r];
					}
				}
			}
		}

	protected:
		AudioBuffer ring;
		WHead1x wHead;
	public:
		int latency;
	};

	struct MixProcessorNonlinear
	{
		MixProcessorNonlinear() :
			gainIn(0.)
		{}

		void prepare(double sampleRate)
		{
			gainIn.prepare(sampleRate);
		}

		void split(double* const* samples, double gainInDb,
			int numChannels, int numSamples) noexcept
		{
			gainIn(samples, gainInDb, numChannels, numSamples);
		}

		void join(double* const* samples,
			int numChannels, int numSamples,
			bool unityGain) noexcept
		{
			if(unityGain)
				gainIn.applyInverse(samples, numChannels, numSamples);
		}

		Gain<13., -120.> gainIn;
	};

	struct MixProcessorDryWet
	{
		MixProcessorDryWet() :
			parallelProcessor(),
			gainDry(0.f),
			gainWetIn(0.f),
			gainWetOut(0.f)
		{}

		void prepare(double sampleRate)
		{
			gainDry.prepare(sampleRate);
			gainWetIn.prepare(sampleRate);
			gainWetOut.prepare(sampleRate);
		}

		/* samples, gainDryDb, numChannels, numSamples */
		void splitLinear(double* const* samples, double gainDryDb,
			int numChannels, int numSamples) noexcept
		{
			parallelProcessor.split(samples, numChannels, numSamples);
			const auto band = parallelProcessor.getBand(0);
			double* bandBuf[] = { band.l, band.r };
			gainDry(bandBuf, gainDryDb, numChannels, numSamples);
		}

		/* samples, gainDryDb, gainWetInDb, numChannels, numSamples */
		void splitNonlinear(double* const* samples, double gainDryDb, double gainWetInDb,
			int numChannels, int numSamples) noexcept
		{
			splitLinear(samples, gainDryDb, numChannels, numSamples);
			gainWetIn(samples, gainWetInDb, numChannels, numSamples);
		}

		/* samples, gainWetDb, numChannels, numSamples  */
		void joinLinear(double* const* samples, double gainWetOutDb,
			int numChannels, int numSamples) noexcept
		{
			gainWetOut(samples, gainWetOutDb, numChannels, numSamples);
			parallelProcessor.join(samples, numChannels, numSamples);
		}

		/* samples, gainWetDb, numChannels, numSamples, unityGain  */
		void joinNonlinear(double* const* samples, double gainWetOutDb,
			int numChannels, int numSamples, bool unityGain) noexcept
		{
			if(unityGain)
				gainWetIn.applyInverse(samples, numChannels, numSamples);
			joinLinear(samples, gainWetOutDb, numChannels, numSamples);
		}

	private:
		PP2Band parallelProcessor;
		Gain<13., -60.> gainDry, gainWetIn, gainWetOut;
	};

	struct MixProcessorWetMix
	{
		MixProcessorWetMix() :
			parallelProcessor(),
			gainWetIn(0.),
			gainWetOut(0.),
			mixPRM(1.)
		{}

		void prepare(double sampleRate)
		{
			gainWetIn.prepare(sampleRate);
			gainWetOut.prepare(sampleRate);
			mixPRM.prepare(sampleRate, 13.);
		}

		/* samples, numChannels, numSamples */
		void splitLinear(double* const* samples,
			int numChannels, int numSamples) noexcept
		{
			parallelProcessor.split(samples, numChannels, numSamples);
		}

		/* samples, gainWetInDb, numChannels, numSamples */
		void splitNonlinear(double* const* samples, double gainWetInDb,
			int numChannels, int numSamples) noexcept
		{
			splitLinear(samples, numChannels, numSamples);
			gainWetIn(samples, gainWetInDb, numChannels, numSamples);
		}

		/* samples, mix, gainWetOutDb, numChannels, numSamples, delta  */
		void joinLinear(double* const* samples, double mix, double gainWetOutDb,
			int numChannels, int numSamples, bool delta) noexcept
		{
			gainWetOut(samples, gainWetOutDb, numChannels, numSamples);
			if (delta)
				joinDelta(samples, mix, numChannels, numSamples);
			else
				joinMix(samples, mix, numChannels, numSamples);
		}

		/* samples, mix, gainWetOutDb, numChannels, numSamples, unityGain, delta  */
		void joinNonlinear(double* const* samples, double mix, double gainWetOutDb,
			int numChannels, int numSamples, bool unityGain, bool delta) noexcept
		{
			if (unityGain)
				gainWetIn.applyInverse(samples, numChannels, numSamples);
			joinLinear(samples, mix, gainWetOutDb, numChannels, numSamples, delta);
		};
	
	private:
		PP2Band parallelProcessor;
		Gain<13., -120.> gainWetIn, gainWetOut;
		PRMD mixPRM;

		/* samples, mix, numChannels, numSamples */
		void joinMix(double* const* samples, double mix, int numChannels, int numSamples) noexcept
		{
			const auto mixInfo = mixPRM(mix, numSamples);
			if (mixInfo.smoothing)
				parallelProcessor.joinMix(samples, mixInfo.buf, numChannels, numSamples);
			else
				parallelProcessor.joinMix(samples, mixInfo.val, numChannels, numSamples);
		}

		/* samples, mix, numChannels, numSamples */
		void joinDelta(double* const* samples, double mix, int numChannels, int numSamples) noexcept
		{
			const auto mixInfo = mixPRM(mix, numSamples);
			if (mixInfo.smoothing)
				parallelProcessor.joinDelta(samples, mixInfo.buf, numChannels, numSamples);
			else
				parallelProcessor.joinDelta(samples, mixInfo.val, numChannels, numSamples);
		}
	};

	struct MixProcessor
	{
		MixProcessor() :
			mixProcessor(),
			gainOut(0.)
		{}

		void prepare(double sampleRate)
		{
			mixProcessor.prepare(sampleRate);
			gainOut.prepare(sampleRate);
		}

#if PPDIO == PPDIOOut
	#if PPDIsNonlinear
		/* samples, gainInDb, numChannels, numSamples */
		void split(double* const* samples, double gainInDb,
			int numChannels, int numSamples) noexcept
		{
			mixProcessor.split(samples, gainInDb, numChannels, numSamples);
		}
		/* samples, gainOutDb, numChannels, numSamples, unityGain */
		void join(double* const* samples, double gainOutDb,
			int numChannels, int numSamples, bool unityGain) noexcept
		{
			mixProcessor.join(samples, numChannels, numSamples, unityGain);
			gainOut(samples, gainOutDb, numChannels, numSamples);
		}
	#else
		void join(double* const* samples, double gainOutDb,
			int numChannels, int numSamples) noexcept
		{
			gainOut(samples, gainOutDb, numChannels, numSamples);
		}
	#endif
#elif PPDIO == PPDIODryWet
	#if PPDIsNonlinear
		/* samples, gainDryDb, gainWetInDb, numChannels, numSamples */
		void split(double* const* samples, double gainDryDb, double gainWetInDb,
			int numChannels, int numSamples) noexcept
		{
			mixProcessor.splitNonlinear(samples, gainDryDb, gainWetInDb, numChannels, numSamples);
		}

		/* samples, gainWetOutDb, gainOutDb, numChannels, numSamples, unityGain */
		void join(double* const* samples, double gainWetOutDb, double gainOutDb,
			int numChannels, int numSamples, bool unityGain) noexcept
		{
			mixProcessor.joinNonlinear(samples, gainWetOutDb, numChannels, numSamples, unityGain);
			gainOut(samples, gainOutDb, numChannels, numSamples);
		}
	#else
		/* samples, gainDryDb, numChannels, numSamples */
		void split(double* const* samples, double gainDryDb,
			int numChannels, int numSamples) noexcept
		{
			mixProcessor.splitLinear(samples, gainDryDb, numChannels, numSamples);
		}

		/* samples, gainWetOutDb, gainOutDb, numChannels, numSamples */
		void join(double* const* samples, double gainWetOutDb, double gainOutDb,
			int numChannels, int numSamples) noexcept
		{
			mixProcessor.joinLinear(samples, gainWetOutDb, numChannels, numSamples);
			gainOut(samples, gainOutDb, numChannels, numSamples);
		}
	#endif
#else
	#if PPDIsNonlinear
		/* samples, gainWetInDb, numChannels, numSamples */
		void split(double* const* samples, double gainWetInDb,
			int numChannels, int numSamples) noexcept
		{
			mixProcessor.splitNonlinear(samples, gainWetInDb, numChannels, numSamples);
		}

		/* samples, mix, gainWetDb, gainOutDb, numChannels, numSamples, unityGain, delta */
		void join(double* const* samples, double mix, double gainWetDb, double gainOutDb,
			int numChannels, int numSamples, bool unityGain, bool delta) noexcept
		{
			mixProcessor.joinNonlinear(samples, mix, gainWetDb, numChannels, numSamples, unityGain, delta);
			gainOut(samples, gainOutDb, numChannels, numSamples);
		}
	#else
		/* samples, numChannels, numSamples */
		void split(double* const* samples,
			int numChannels, int numSamples) noexcept
		{
			mixProcessor.splitLinear(samples, numChannels, numSamples);
		}

		/* samples, mix, gainWetDb, gainOutDb, numChannels, numSamples, delta */
		void join(double* const* samples, double mix, double gainWetDb, double gainOutDb,
			int numChannels, int numSamples, bool delta) noexcept
		{
			mixProcessor.joinLinear(samples, mix, gainWetDb, numChannels, numSamples, delta);
			gainOut(samples, gainOutDb, numChannels, numSamples);
		}
	#endif
#endif

	private:
#if PPDIO == PPDIOOut
		MixProcessorNonlinear mixProcessor;
#elif PPDIO == PPDIODryWet
		MixProcessorDryWet mixProcessor;
#else
		MixProcessorWetMix mixProcessor;
#endif
		Gain<13., -60.> gainOut;
	};
}