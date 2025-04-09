#pragma once
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#define oopsie(x) jassert(!(x))

namespace math
{
    using Char = juce::juce_wchar;
	using String = juce::String;
	using Point = juce::Point<int>;
	using PointF = juce::Point<float>;
	
    static constexpr double Tau = 6.28318530718;
    static constexpr double Pi = 3.14159265359;
    static constexpr double PiInv = 1. / Pi;
    static constexpr double PiHalf = Pi * .5;
    static constexpr double PiHalfInv = 1. / PiHalf;

    inline int fibonacci(int iterations) noexcept
    {
		int a = 0, b = 1;
		for (auto i = 0; i < iterations; ++i)
		{
			const auto temp = a;
			a = b;
			b += temp;
		}
		return a;
    }

    inline bool isPrime(int n) noexcept
    {
        const auto numSqrt = std::sqrt(n);
        for (auto i = 2; i <= numSqrt; ++i)
        {
            if (n % i == 0)
                return false;
        }
        return true;
    }

    inline int prime(int iterations) noexcept
    {
		int n = 2;
        while(true)
		{
			if (isPrime(n))
			{
                --iterations;
				if (iterations == 0)
					return n;
			}
			++n;
		}
    }

    template<typename Float>
    inline bool bufferSilent(Float* smpls, int numSamples) noexcept
    {
        static constexpr auto Eps = static_cast<Float>(1e-6);
        for (auto s = 0; s < numSamples; ++s)
        {
            const auto smpl = smpls[s];
            if (smpl * smpl > Eps)
                return false;
        }
        return true;
    }

    template<typename Float>
    inline bool bufferSilent(Float** samples, int numChannels, int numSamples) noexcept
    {
        static constexpr auto Eps = static_cast<Float>(1e-6);
        for (auto ch = 0; ch < numChannels; ++ch)
			if (!bufferSilent(samples[ch], numSamples))
				return false;
        return true;
    }

    template<typename Float>
    inline Float limit(Float min, Float max, Float x) noexcept
    {
		return x < min ? min : x > max ? max : x;
    }

    template<typename Float>
    inline Float sinApprox(Float x) noexcept
    {
        const auto x2 = x * x;
        const auto numerator = -x * (static_cast<Float>(-11511339840) + x2 * (static_cast<Float>(1640635920) + x2 * (static_cast<Float>(-52785432) + x2 * static_cast<Float>(479249))));
        const auto denominator = static_cast<Float>(11511339840) + x2 * (static_cast<Float>(277920720) + x2 * (static_cast<Float>(3177720) + x2 * static_cast<Float>(18361)));
        return numerator / denominator;
    }

	template<typename Float>
    inline Float cosApprox(Float x) noexcept
    {
        return sinApprox(x + static_cast<Float>(PiHalf));
    }

    // for x != -9652.5
    template<typename Float>
    inline Float tanhApprox(Float x) noexcept
    {
		const auto x2 = x * x;
		const auto numerator = x * (static_cast<Float>(135135) + x2 * (static_cast<Float>(17325) + x2 * static_cast<Float>(378)));
		const auto denominator = static_cast<Float>(135135) + x2 * (static_cast<Float>(62370) + x2 * (static_cast<Float>(3150) + x2 * static_cast<Float>(28)));
		return numerator / denominator;
    }

    inline float invSqrt(float x) noexcept
    {
		union { float f; int i; } y;
		y.f = x;
		y.i = 0x5f3759df - (y.i >> 1);
		return y.f * (1.5f - 0.5f * x * y.f * y.f);
    }

    // the quake III hack
	inline float invSqrtQuake(float x) noexcept
	{
		long i;
		float x2, y;
		const float threehalfs = 1.5F;

		x2 = x * .5F;
		y = x;
		i = *(long*)&y;
		i = 0x5f3759df - (i >> 1);
		y = *(float*)&i;
		y = y * (threehalfs - (x2 * y * y));
		return y;
	}

    // not smooth
    inline float expApprox(float x) noexcept
    {
        union { float f; int i; } y;
        y.i = static_cast<int>(x * 0xB5645F + 0x3F7893F5);
        return (y.f);
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
    inline Float samplesToSecs(Float samples, Float fsInv) noexcept
    {
		return samples * fsInv;
    }

    template<typename Float>
    inline Float msToSamples(Float ms, Float Fs) noexcept
    {
        return secsToSamples(ms * static_cast<Float>(.001), Fs);
    }

    template<typename Float>
    inline Float samplesToMs(Float samples, Float fsInv) noexcept
    {
		return samplesToSecs(samples, fsInv) * static_cast<Float>(1000);
    }


    template<typename Float>
    inline Float msToInc(Float ms, Float Fs) noexcept
    {
        return static_cast<Float>(1) / msToSamples(ms, Fs);
    }

    template<typename Float>
    inline Float incToMs(Float inc, Float Fs) noexcept
    {
		return static_cast<Float>(1000) / inc;
    }

    template<typename Float>
    inline Float freqHzToSamples(Float hz, Float Fs) noexcept
    {
        return Fs / hz;
    }

    template<typename Float>
    inline Float getRMS(const Float* ar, const int size) noexcept
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
            return static_cast<Float>(0);
        return dbToAmp(db);
    }

    template<typename Float>
	inline Float lerp(const Float* buffer, Float readHead) noexcept
	{
		const auto iFloor = std::floor(readHead);
		auto i0 = static_cast<int>(iFloor);
		auto i1 = i0 + 1;
		const auto t = readHead - iFloor;
		const auto v0 = buffer[i0];
		const auto v1 = buffer[i1];
		return v0 + t * (v1 - v0);
	}

    template<typename Float>
	inline Float lerp(const Float* buffer, Float readHead, int size) noexcept
	{
		const auto iFloor = std::floor(readHead);
		auto i0 = static_cast<int>(iFloor);
		auto i1 = i0 + 1;
		if (i1 >= size) i1 -= size;
		const auto t = readHead - iFloor;
		const auto v0 = buffer[i0];
		const auto v1 = buffer[i1];
		return v0 + t * (v1 - v0);
	}

    template<typename T>
    inline T cubicHermiteSpline(const T* buffer, T readHead, int size) noexcept
    {
        const auto iFloor = std::floor(readHead);
        auto i1 = static_cast<int>(iFloor);
        auto i0 = i1 - 1;
        auto i2 = i1 + 1;
        auto i3 = i1 + 2;
        if (i3 >= size) i3 -= size;
        if (i2 >= size) i2 -= size;
        if (i0 < 0) i0 += size;

        const auto t = readHead - iFloor;
        const auto v0 = buffer[i0];
        const auto v1 = buffer[i1];
        const auto v2 = buffer[i2];
        const auto v3 = buffer[i3];

        const auto c0 = v1;
        const auto c1 = static_cast<T>(.5) * (v2 - v0);
        const auto c2 = v0 - static_cast<T>(2.5) * v1 + static_cast<T>(2.) * v2 - static_cast<T>(.5) * v3;
        const auto c3 = static_cast<T>(1.5) * (v1 - v2) + static_cast<T>(.5) * (v3 - v0);

        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    template<typename T>
    inline T cubicHermiteSpline(const T* buffer, T readHead) noexcept
    {
        const auto iFloor = std::floor(readHead);
        auto i0 = static_cast<int>(iFloor);
        auto i1 = i0 + 1;
        auto i2 = i0 + 2;
        auto i3 = i0 + 3;

        const auto t = readHead - iFloor;
        const auto v0 = buffer[i0];
        const auto v1 = buffer[i1];
        const auto v2 = buffer[i2];
        const auto v3 = buffer[i3];

        const auto c0 = v1;
        const auto c1 = static_cast<T>(.5) * (v2 - v0);
        const auto c2 = v0 - static_cast<T>(2.5) * v1 + static_cast<T>(2.) * v2 - static_cast<T>(.5) * v3;
        const auto c3 = static_cast<T>(1.5) * (v1 - v2) + static_cast<T>(.5) * (v3 - v0);

        return ((c3 * t + c2) * t + c1) * t + c0;
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