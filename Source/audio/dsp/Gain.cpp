#include "Gain.h"
#include "../../arch/Math.h"

namespace dsp
{
	template<double SmoothLengthMs, double MinDb>
	Gain<SmoothLengthMs, MinDb>::Gain(double defaultValueDb) :
		gainPRM(math::dbToAmp(defaultValueDb, MinDb)),
		gainInfo(nullptr, 0., false)
	{}

	template<double SmoothLengthMs, double MinDb>
	void Gain<SmoothLengthMs, MinDb>::prepare(double sampleRate) noexcept
	{
		gainPRM.prepare(sampleRate, SmoothLengthMs);
	}

	template<double SmoothLengthMs, double MinDb>
	void Gain<SmoothLengthMs, MinDb>::operator()(double* const* samples, double gainDb,
		int numChannels, int numSamples) noexcept
	{
		const auto gainAmp = math::dbToAmp(gainDb, MinDb);
		gainInfo = gainPRM(gainAmp, numSamples);

		if (gainInfo.smoothing)
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				SIMD::multiply(smpls, gainInfo.buf, numSamples);
			}
		else if(gainInfo.val != 1.)
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				SIMD::multiply(smpls, gainInfo.val, numSamples);
			}
	}

	template<double SmoothLengthMs, double MinDb>
	void Gain<SmoothLengthMs, MinDb>::applyInverse(double* const* samples, int numChannels, int numSamples) noexcept
	{
		if (gainInfo.smoothing)
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				for (auto s = 0; s < numSamples; ++s)
					smpls[s] /= gainInfo.buf[s];
			}
		else
		{
			gainInfo.val = 1. / gainInfo.val;

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				SIMD::multiply(smpls, gainInfo.val, numSamples);
			}
		}
	}

	template<double SmoothLengthMs, double MinDb>
	void Gain<SmoothLengthMs, MinDb>::applyInverse(double* smpls, int numSamples) noexcept
	{
		if (gainInfo.smoothing)
			for (auto s = 0; s < numSamples; ++s)
				smpls[s] /= gainInfo.buf[s];
		else
		{
			gainInfo.val = 1. / gainInfo.val;
			SIMD::multiply(smpls, gainInfo.val, numSamples);
		}
	}

	template struct Gain<1., -60.>;
	template struct Gain<2., -60.>;
	template struct Gain<3., -60.>;
	template struct Gain<5., -60.>;
	template struct Gain<8., -60.>;
	template struct Gain<13., -60.>;
	template struct Gain<21., -60.>;
	template struct Gain<34., -60.>;
	template struct Gain<55., -60.>;
	template struct Gain<89., -60.>;
	template struct Gain<144., -60.>;

	template struct Gain<1., -120.>;
	template struct Gain<2., -120.>;
	template struct Gain<3., -120.>;
	template struct Gain<5., -120.>;
	template struct Gain<8., -120.>;
	template struct Gain<13., -120.>;
	template struct Gain<21., -120.>;
	template struct Gain<34., -120.>;
	template struct Gain<55., -120.>;
	template struct Gain<89., -120.>;
	template struct Gain<144., -120.>;
}