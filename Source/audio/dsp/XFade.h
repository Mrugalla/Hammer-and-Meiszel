#pragma once
#include "../Using.h"

namespace dsp
{
    struct XFade
    {
        XFade() :
            buffer(),
            phase(0.),
            inc(0.),
            idx(0),
            fading(false)
        {
        }

        void prepare(double sampleRate, double lengthMs, int blockSize)
        {
            buffer.setSize(3, blockSize, false, true, false);
            inc = math::msToInc(lengthMs, sampleRate);
        }

        void init() noexcept
        {
            idx = 1 - idx;
            phase = 0.;
            fading = true;
        }

        double* const* getSamples() noexcept
        {
            return buffer.getArrayOfWritePointers();
        }

        void synthesizePhase(int numSamples) noexcept
        {
            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto s = 0; s < numSamples; ++s)
            {
                xBuf[s] = static_cast<float>(phase);
                phase += inc;
                if (phase > 1.)
                {
                    fading = false;
                    phase = 1.;
                    for (; s < numSamples; ++s)
                        xBuf[s] = 1.f;
                    return;
                }
            }
        }

        void operator()(double* const* samples, int numChannels, int numSamples) noexcept
        {
            synthesizePhase(numSamples);

            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto ch = 0; ch < numChannels; ++ch)
            {
                auto smpls = samples[ch];
                const auto xSmpls = xSamples[ch];

                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto smpl = smpls[s];
                    const auto xSmpl = xSmpls[s];

                    const auto xFade = xBuf[s];
                    const auto xPi = xFade * Pi;
                    const auto frac = std::cos(xPi + Pi) + 1.f;
                    const auto xFrac = std::cos(xPi) + 1.f;

                    smpls[s] = (smpl * frac + xSmpl * xFrac) * .5f;
                }
            }
        }

        AudioBuffer buffer;
        double phase, inc;
        int idx;
        bool fading;
    };

    template<size_t NumTracks, bool Smooth>
    struct XFadeMixer
    {
        using Buffer = std::array<double*, 3>;

        struct Track
        {
            Track() :
                gain(0.),
                destGain(0.),
                inc(0.),
                fading(false)
            {
            }

            void disable() noexcept
            {
                destGain = 0.;
            }

            void enable() noexcept
            {
                destGain = 1.;
            }

            const bool isEnabled() const noexcept
            {
                return gain + destGain != 0.;
            }

            const bool isFading() const noexcept
            {
                return destGain != gain;
            }

            void synthesizeGainValues(double* xBuf, int numSamples) noexcept
            {
                if (!isFading())
                {
                    SIMD::fill(xBuf, gain, numSamples);
                    fading = false;
                    return;
                }

                fading = true;
                synthesizeGainValuesInternal(xBuf, numSamples);

                if (Smooth)
                    makeSmooth(xBuf, numSamples);
            }

            void copy(double* const* dest, const double* const* src,
                int numChannels, int numSamples) const noexcept
            {
                if (fading)
                {
                    const auto xBuf = src[2];
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::multiply(dest[ch], src[ch], xBuf, numSamples);
                }
                else if (gain == 1.)
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::copy(dest[ch], src[ch], numSamples);
            }

            void copy(double* dest, const double* const* src,
                int numSamples) const noexcept
            {
                if (fading)
                {
                    const auto xBuf = src[2];
                    SIMD::multiply(dest, src[0], xBuf, numSamples);
                }
                else if (gain == 1.)
                    SIMD::copy(dest, src[0], numSamples);
            }

            void add(double* const* dest, const double* const* src,
                int numChannels, int numSamples) const noexcept
            {
                if (fading)
                {
                    const auto xBuf = src[2];
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::addWithMultiply(dest[ch], src[ch], xBuf, numSamples);
                }
                else if (gain == 1.)
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::add(dest[ch], src[ch], numSamples);
            }

            void add(double* dest, const double* const* src,
                int numSamples) const noexcept
			{
				if (fading)
				{
					const auto xBuf = src[2];
					SIMD::addWithMultiply(dest, src[0], xBuf, numSamples);
				}
				else if (gain == 1.)
					SIMD::add(dest, src[0], numSamples);
			}

            double gain, destGain, inc;
        protected:
            bool fading;

            void synthesizeGainValuesInternal(double* xBuf, int numSamples) noexcept
            {
                if (destGain == 1.)
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        xBuf[s] = gain;
                        gain += inc;
                        if (gain >= 1.)
                        {
                            gain = 1.;
                            for (; s < numSamples; ++s)
                                xBuf[s] = gain;
                            return;
                        }
                    }
                else
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        xBuf[s] = gain;
                        gain -= inc;
                        if (gain < 0.)
                        {
                            gain = 0.;
                            for (; s < numSamples; ++s)
                                xBuf[s] = gain;
                            return;
                        }
                    }
            }

            void makeSmooth(double* xBuf, int numSamples) const noexcept
            {
                for (auto s = 0; s < numSamples; ++s)
                    xBuf[s] = std::cos(xBuf[s] * Pi + Pi) * .5 + .5;
            }
        };

        XFadeMixer() :
            buffer(),
            tracks(),
            idx(0)
        {
        }

        void prepare(double sampleRate, double lengthMs)
        {
            for(auto& b: buffer)
				for (auto& x : b)
					x = 0.;
            const auto inc = math::msToInc(lengthMs, sampleRate);
            for (auto& track : tracks)
            {
                track.gain = 0.;
                track.inc = inc;
            }
            tracks[idx].gain = 1.;
        }

        void init() noexcept
        {
            idx = (idx + 1) % NumTracks;
            for (auto& track : tracks)
                track.disable();
            tracks[idx].enable();
        }

        Buffer getBuffer(int i) noexcept
        {
			return { buffer[i * 3].data(), buffer[i * 3 + 1].data(), buffer[i * 3 + 2].data() };
        }

        const int numTracksEnabled() const noexcept
        {
            auto sum = 0;
            for (auto i = 0; i < NumTracks; ++i)
                sum += (tracks[i].isEnabled() ? 1 : 0);
            return sum;
        }

        Track& operator[](int i) noexcept
        {
            return tracks[i];
        }

        bool stillFading() const noexcept
        {
            return tracks[idx].gain != 1.;
        }

    protected:
        std::array<std::array<double, BlockSize>, NumTracks * 3> buffer;
        std::array<Track, NumTracks> tracks;
    public:
        int idx;
    };

}