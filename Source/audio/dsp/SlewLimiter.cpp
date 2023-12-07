#include "SlewLimiter.h"

namespace dsp
{
    SlewLimiter::SlewLimiter() :
        y(0.),
        dist(0.),
        filterTypeFuncs()
    {
        filterTypeFuncs[LP] = [&x = y]()
        {
            return x;
        };
        filterTypeFuncs[HP] = [&x = dist]()
        {
            return x;
        };
    }

    void SlewLimiter::operator()(double* smpls, double slew,
        int numSamples, Type type) noexcept
    {
        for (auto s = 0; s < numSamples; ++s)
            smpls[s] = process(smpls[s], slew, type);
    }

    double SlewLimiter::process(double x, double slew, Type type) noexcept
    {
        dist = x - y;
        const auto pol = dist < 0. ? -1. : 1.;
        y += dist * pol > slew ? slew * pol : dist;
        return filterTypeFuncs[type]();
    }

    double SlewLimiterStereo::freqHzToSlewRate(double freq, double sampleRate) noexcept
    {
        return freq / sampleRate;
    }

    SlewLimiterStereo::SlewLimiterStereo() :
        slews(),
        sampleRate(1.)
    {}

    void SlewLimiterStereo::operator()(double** samples, double slew,
        int numChannels, int numSamples, Type type) noexcept
    {
        for (auto ch = 0; ch < numChannels; ++ch)
            slews[ch](samples[ch], slew, numSamples, type);
    }
}