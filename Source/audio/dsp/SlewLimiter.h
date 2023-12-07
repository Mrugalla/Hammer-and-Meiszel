#pragma once
#include "../Using.h"

namespace dsp
{
	struct SlewLimiter
	{
        enum Type { LP, HP, NumTypes };
        using FilerTypeFunc = std::function<double()>;
        using FilterTypeFuncs = std::array<FilerTypeFunc, NumTypes>;
       
        SlewLimiter();

        /* smpls, slew[0,2], numSamples, type */
        void operator()(double* smpls, double slew,
            int numSamples, Type type) noexcept;

    protected:
        double y, dist;
        FilterTypeFuncs filterTypeFuncs;

        /* x, slew, type */
        double process(double, double, Type) noexcept;
	};

    struct SlewLimiterStereo
    {
        using Type = SlewLimiter::Type;

        /* freq, sampleRate */
        static double freqHzToSlewRate(double, double) noexcept;

        SlewLimiterStereo();

        /* samples, slewRate[0,2], numChannels, numSamples, filterType */
        void operator()(double**, double, int, int, Type) noexcept;

    protected:
        std::array<SlewLimiter, 2> slews;
        double sampleRate;
    };
}