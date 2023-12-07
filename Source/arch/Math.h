#pragma once
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

namespace math
{
    using Char = juce::juce_wchar;
	using String = juce::String;
	using Point = juce::Point<int>;
	using PointF = juce::Point<float>;
	
    static constexpr float Tau = 6.28318530718f;
    static constexpr float Pi = 3.14159265359f;
    static constexpr float PiInv = 1.f / Pi;
    static constexpr float PiHalf = Pi * .5f;
    static constexpr float PiHalfInv = 1.f / PiHalf;

    template <typename Float>
    inline Float sinApprox(Float x) noexcept
    {
        const auto x2 = x * x;
        const auto numerator = -x * (static_cast<Float>(-11511339840) + x2 * (static_cast<Float>(1640635920) + x2 * (static_cast<Float>(-52785432) + x2 * static_cast<Float>(479249))));
        const auto denominator = static_cast<Float>(11511339840) + x2 * (static_cast<Float>(277920720) + x2 * (static_cast<Float>(3177720) + x2 * static_cast<Float>(18361)));
        return numerator / denominator;
    }

    template <typename Float>
    inline Float slightlySmaller(Float x) noexcept
    {
        return x * (static_cast<Float>(1) - std::numeric_limits<Float>::epsilon());
    }

    template<typename Float>
    inline Float secsToSamples(Float secs, Float Fs) noexcept
    {
        return secs * Fs;
    }

    template<typename Float>
    inline Float msToSamples(Float ms, Float Fs) noexcept
    {
        return secsInSamples(ms * static_cast<Float>(.001), Fs);
    }

    template<typename Float>
    inline Float msToInc(Float ms, Float Fs) noexcept
    {
        return static_cast<Float>(1) / msInSamples(ms, Fs);
    }

    template<typename Float>
    inline Float freqHzToSamples(Float hz, Float Fs) noexcept
    {
        return Fs / hz;
    }

    template<typename Float>
    inline float getRMS(const Float* ar, const int size) noexcept
    {
        auto rms = static_cast<Float>(0);
        for (auto i = 0; i < size; ++i)
            rms += ar[i] * ar[i];
        rms /= static_cast<Float>(size);
        return std::sqrt(rms);
    }

    template<typename Float>
    inline Float noteToFreqHz(Float note, Float rootNote = static_cast<Float>(69), Float xen = static_cast<Float>(12), Float masterTune = static_cast<Float>(440)) noexcept
    {
        return std::exp2((note - rootNote) / xen) * masterTune;
    }

    template<typename Float>
    inline Float noteToFreqHz2(Float note, Float rootNote = static_cast<Float>(69), Float masterTune = static_cast<Float>(440)) noexcept
    {
        return std::exp2((note - rootNote) * static_cast<Float>(.08333333333)) * masterTune;
    }

    template<typename Float>
    inline Float freqHzToNote(Float freqHz, Float rootNote = static_cast<Float>(69), Float xen = static_cast<Float>(12), Float masterTune = static_cast<Float>(440)) noexcept
    {
        return std::log2(freqHz / masterTune) * xen + rootNote;
    }

    template<typename Float>
    inline Float freqHzToNote2(Float freqHz, Float xen = static_cast<Float>(12), Float rootNote = static_cast<Float>(69)) noexcept
    {
        return std::log2(freqHz * static_cast<Float>(.00227272727)) * xen + rootNote;
    }

    template<typename Float>
    Float closestFreq(Float freq, Float xen = static_cast<Float>(12), Float basePitch = static_cast<Float>(69), Float masterTune = static_cast<Float>(440)) noexcept
    {
        auto closestFreq = static_cast<Float>(0);
        auto closestDist = std::numeric_limits<Float>::max();

        for (auto note = 0; note < 128; ++note)
        {
            const auto nFreq = noteToFreqHz(static_cast<Float>(note), xen, basePitch, masterTune);
            const auto nDist = std::abs(freq - nFreq);
            if (nDist < closestDist)
            {
                closestDist = nDist;
                closestFreq = nFreq;
            }
        }

        return closestFreq;
    }

    template<typename Float>
    inline Float freqHzToFc(Float freq, Float Fs) noexcept
    {
        return freq / Fs;
    }

    template<typename Float>
    inline Float fcToFreqHz(Float fc, Float Fs) noexcept
    {
        return fc * Fs;
    }

    template<typename Float>
    inline Float ampToDecibel(Float x) noexcept
    {
        return std::log10(x) * static_cast<Float>(20);
    }

    template<typename Float>
    inline Float dbToAmp(Float db) noexcept
    {
        return std::pow(static_cast<Float>(10), db * static_cast<Float>(.05));
    }

    template<typename Float>
    inline Float dbToAmp(Float db, Float threshold) noexcept
    {
        if (db <= threshold)
            return 0.f;
        return dbToAmp(db);
    }

    /* oct [-n, n], semi [-12, 12], fine [-1, 1]*/
    template<typename Float>
    inline Float getRetuneValue(Float oct, Float semi, Float fine) noexcept
    {
        return static_cast<Float>(12) * std::round(oct) + std::round(semi) + fine;
    }

    inline bool isDigit(Char chr) noexcept
    {
        return chr >= '0' && chr <= '9';
    }

    inline bool isLetter(Char chr) noexcept
    {
        return (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z');
    }

    inline bool isLetterOrDigit(Char chr) noexcept
    {
        return isLetter(chr) || isDigit(chr);
    }

    inline int getDigit(Char chr) noexcept
    {
        return chr - '0';
    }

    inline String pitchclassToString(int pitchclass) noexcept
    {
        switch (pitchclass)
        {
        case 0: return "C";
        case 1: return "C#";
        case 2: return "D";
        case 3: return "D#";
        case 4: return "E";
        case 5: return "F";
        case 6: return "F#";
        case 7: return "G";
        case 8: return "G#";
        case 9: return "A";
        case 10: return "A#";
        case 11: return "B";
        default: return "C";
        }
    }

    inline bool isWhiteKey(int pitchclass) noexcept
    {
        switch (pitchclass)
        {
        case 0: return true;
        case 1: return false;
        case 2: return true;
        case 3: return false;
        case 4: return true;
        case 5: return true;
        case 6: return false;
        case 7: return true;
        case 8: return false;
        case 9: return true;
        case 10: return false;
        case 11: return true;
        default: return true;
        }
    }

    inline bool isBlackKey(int pitchclass) noexcept
    {
        return !isWhiteKey(pitchclass);
    }

    template<typename Float>
    void applySomeWindowingFunction(Float* buffer, int size) noexcept
    {
    https://www.desmos.com/calculator/qzrswwvqfo
        const auto a0 = static_cast<Float>(0.35875f);
        const auto a1 = static_cast<Float>(0.48829f);
        const auto a2 = static_cast<Float>(0.14128f);
        const auto a3 = static_cast<Float>(0.01168f);
        const auto inc = Tau / static_cast<Float>(size);
        auto x = static_cast<Float>(0);
        for (auto i = 0; i < size; ++i, x += inc)
        {
            const auto w0 = a1 * std::cos(x);
            const auto w1 = a2 * std::cos(static_cast<Float>(2) * x);
            const auto w2 = a3 * std::cos(static_cast<Float>(3) * x);
            const auto w = a0 - w0 + w1 - a3 * w2;
            buffer[i] *= w;
        }
    }

    inline float getNumBeats(const String& str)
    {
        for (auto i = 0; i < str.length(); ++i)
        {
            auto chr = str[i];
            if (chr == '/')
            {
                auto num = str.substring(0, i).getFloatValue();
                auto den = str.substring(i + 1).getFloatValue();
                return num / den;
            }
        }
        return -1.f;
    }

    inline float nextLowestPowTwoX(float x) noexcept
    {
        return std::pow(2.f, std::floor(std::log2(x)));
    }

    inline bool stringNegates(const String& t)
    {
        return t == "off"
            || t == "false"
            || t == "no"
            || t == "0"
            || t == "disabled"
            || t == "none"
            || t == "null"
            || t == "nil"
            || t == "nada"
            || t == "nix"
            || t == "nichts"
            || t == "niente"
            || t == "nope"
            || t == "nay"
            || t == "nein"
            || t == "njet"
            || t == "nicht"
            || t == "nichts";
    }

	inline Point getAspectRatio(const String& str)
	{
		const auto colon = str.indexOfChar(':');
		if (colon <= 0)
			return { 1, 1 };
		
		const auto w = str.substring(0, colon).getIntValue();
		const auto h = str.substring(colon + 1).getIntValue();
		return { w, h };
	}
}