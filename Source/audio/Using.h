#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../arch/Math.h"
#include <array>

namespace dsp
{
	static constexpr double Pi = 3.1415926535897932384626433832795;
	static constexpr double Tau = Pi * 2.;
	static constexpr double PiHalf = Pi * .5;
	static constexpr double E = 2.7182818284590452353602874713527;
	static constexpr double TauInv = 1. / Tau;

	static constexpr float PiF = 3.1415926535897932384626433832795f;
	static constexpr float TauF = PiF * 2.f;
	static constexpr double SixtyInv = 1. / 60.;

	using MidiBuffer = juce::MidiBuffer;
	using MidiMessage = juce::MidiMessage;
	using SIMD = juce::FloatVectorOperations;
	using AudioBuffer = juce::AudioBuffer<double>;
	using AudioBufferF = juce::AudioBuffer<float>;
	using Uint8 = uint8_t;
	using Int64 = juce::int64;
	using PlayHead = juce::AudioPlayHead;
	using PosInfo = PlayHead::CurrentPositionInfo;

	static constexpr int NumMIDIChannels = 16;
	static constexpr int NumMPEChannels = NumMIDIChannels - 1;
	static constexpr double PitchbendRange = 16383.;
	static constexpr double PitchbendRangeHalf = PitchbendRange * .5;
	
	enum class OversamplingOrder { x1, x2, x4, NumOrders };
	inline constexpr int getOversamplingFactor(OversamplingOrder order) noexcept
	{
		switch (order)
		{
			case OversamplingOrder::x1: return 1;
			case OversamplingOrder::x2: return 2;
			case OversamplingOrder::x4: return 4;
			default: return 1;
		}
	}

	static constexpr int NumChannels = PPDHasSidechain ? 4 : 2;
    static constexpr int BlockSize = 16;
	static constexpr int BlockSize2x = BlockSize * getOversamplingFactor(OversamplingOrder::x2);
	static constexpr int BlockSize4x = BlockSize * getOversamplingFactor(OversamplingOrder::x4);

	template<typename Float>
	inline void copy(Float* dest, Float* src, int numSamples) noexcept
	{
		SIMD::copy(dest, src, numSamples);
	}

	template<typename Float>
	inline void add(Float* dest, Float* src, int numSamples) noexcept
	{
		SIMD::add(dest, src, numSamples);
	}

	template<typename Float>
	inline void copy(Float** dest, Float** src,
		int numChannels, int numSamples) noexcept
	{
		for(auto ch = 0; ch < numChannels; ++ch)
			copy(dest[ch], src[ch], numSamples);
	}

	template<typename Float>
	inline void add(Float** dest, Float** src,
		int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
			add(dest[ch], src[ch], numSamples);
	}

	using MemoryInputStream = juce::MemoryInputStream;
	using WavAudioFormat = juce::WavAudioFormat;
	using AudioFormatReader = juce::AudioFormatReader;

	inline double loadFromMemory(AudioBufferF& buffer, const char* data, int size)
	{
		double sampleRate = 0.;
		auto stream = new MemoryInputStream(data, size, false);
		WavAudioFormat wav;
		const auto reader = wav.createReaderFor(stream, false);
		if (reader != nullptr)
		{
			sampleRate = reader->sampleRate;
			const auto numChannels = static_cast<int>(reader->numChannels);
			const auto numSamples = static_cast<int>(reader->lengthInSamples);
			buffer.setSize(numChannels, numSamples, false, false, false);
			reader->read(&buffer, 0, numSamples, 0, true, true);
		}
		delete reader;
		return sampleRate;
	}

	using FileOutputStream = juce::FileOutputStream;
	using PNGImageFormat = juce::PNGImageFormat;
	using File = juce::File;
	using Image = juce::Image;
	using Graphics = juce::Graphics;
	using String = juce::String;
	using BoundsF = juce::Rectangle<float>;

	inline void makeImage(Graphics& g, Image& img, const String& name, const String& info)
	{
		const auto fontSize = img.getHeight() * .02f;
		g.setFont(fontSize);
		g.drawFittedText(info, img.getBounds(), juce::Justification::topRight, 10);

		const auto desktop = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory);
		const auto file = desktop.getChildFile(name + ".png");
		if (file.existsAsFile())
			file.deleteFile();
		file.create();
		FileOutputStream stream(file);
		PNGImageFormat pngWriter;
		pngWriter.writeImageToStream(img, stream);
	}

	template<typename Float>
	inline void makeImage(const Float* smpls, int numSamples,
		float lowLim, float upLim, String&& name, bool withDb)
	{
		const auto width = numSamples;
		const auto height = width * 9 / 16;
		const auto heightF = static_cast<float>(height);
		Image img(Image::RGB, width, height, false);
		Graphics g(img);
		g.fillAll(juce::Colours::black);
		g.setColour(juce::Colours::limegreen);
		if (withDb)
		{
			lowLim = lowLim == 0.f ? -60.f : math::ampToDecibel(lowLim);
			upLim = upLim == 0.f ? -60.f : math::ampToDecibel(upLim);
		}
		const auto range = upLim - lowLim;
		for (auto x = 0; x < numSamples; ++x)
		{
			const auto xF = static_cast<float>(x);
			const auto smpl = static_cast<float>(smpls[x]);
			if (withDb)
			{
				if (smpl != 0.f)
				{
					const auto smplDb = juce::jlimit(lowLim, upLim, math::ampToDecibel(smpl));
					const auto smplNorm = (smplDb - lowLim) / range;
					const auto yF = heightF - heightF * smplNorm;
					g.fillRect(xF, yF, 1.f, heightF - yF);
				}
			}
			else
			{
				const auto smplNorm = (smpl - lowLim) / range;
				if (smplNorm != 0.f)
				{
					const auto yF = heightF - heightF * smplNorm;
					g.fillRect(xF, yF, 1.f, heightF - yF);
				}
			}
		}
		
		String info(name + "\nmin: " + String(lowLim) + "\nmax: " + String(upLim));
		makeImage(g, img, name, info);
	}

	template<typename Float>
	inline void makeImageDb(const Float* smpls, int numSamples, float lowDb, float highDb, String&& name)
	{
		const auto width = numSamples;
		const auto height = width * 9 / 16;
		const auto heightF = static_cast<float>(height);
		Image img(Image::RGB, width, height, false);
		Graphics g(img);
		g.fillAll(juce::Colours::black);
		const auto dbRange = highDb - lowDb;
		g.setColour(juce::Colours::limegreen); 
		for (auto x = 0; x < numSamples; ++x)
		{
			const auto xF = static_cast<float>(x);
			const auto smpl = static_cast<float>(smpls[x]);
			if (smpl != 0.f)
			{
				const auto smplDb = math::ampToDecibel(smpl);
				const auto smplNorm = juce::jlimit(0.f, 1.f, (smplDb - lowDb) / dbRange);
				auto yF = heightF - heightF * smplNorm;
				g.fillRect(xF, yF, 1.f, heightF - yF);
			}
		}

		String info(name + "\nmin db: " + String(lowDb) + "\nmax db: " + String(highDb));
		makeImage(g, img, name, info);
	}

	template<typename Float>
	inline void makeImage(const Float* smpls, int numSamples, String&& name, bool withDb)
	{
		auto upLim = 0.f;
		for(auto s = 0; s < numSamples; ++s)
			upLim = std::max(upLim, std::abs(smpls[s]));
		auto lowLim = upLim;
		for (auto s = 0; s < numSamples; ++s)
			lowLim = std::min(lowLim, std::abs(smpls[s]));
		makeImage(smpls, numSamples, lowLim, upLim, std::move(name), withDb);
	}

	template<typename Float>
	inline void makeSpectrumImage(const String& name, const Float* bins, int fftSize,
		float lowDb = -60.f, bool downsample = false, const int* peakIndexes = nullptr, int numPeaks = 0,
		bool drawDebugText = true, bool logScaled = true, float sampleRate = 44100.f)
	{
		const auto maxPitch = 128.f;
		const auto upDb = 0.f;
		const auto width = fftSize;
		const auto widthF = static_cast<float>(width);
		const auto height = width * 9 / 16;
		const auto heightF = static_cast<float>(height);
		Image img(Image::RGB, width, height, false);
		Graphics g(img);
		g.fillAll(juce::Colours::black);
		g.setColour(juce::Colours::limegreen);
		const auto range = upDb - lowDb;
		const auto rangeInv = 1.f / range;

		std::function<int(float)> getBinIdx;
		if (logScaled)
			getBinIdx = [&](float xF)
			{
				const auto xRatio = xF / widthF;
				const auto xPitch = xRatio * maxPitch;
				const auto xFreq = math::noteToFreqHz(xPitch);
				const auto xFreqRatio = xFreq / sampleRate;
				const auto xFreqBinIdx = xFreqRatio * widthF;
				return static_cast<int>(xFreqBinIdx);
			};
		else
			getBinIdx = [&](float xF) { return static_cast<int>(xF); };

		for (auto x = 0; x < width; ++x)
		{
			const auto xF = static_cast<float>(x);
			const auto binIdx = getBinIdx(xF);
			const auto bin = static_cast<float>(bins[binIdx]);
			const auto absBin = std::abs(bin);
			const auto eps = std::numeric_limits<float>::epsilon();
			if (absBin > eps)
			{
				auto binDb = math::ampToDecibel(absBin);
				if (binDb > lowDb)
				{
					binDb = binDb > upDb ? upDb : binDb;
					const auto binNorm = (binDb - lowDb) * rangeInv;
					const auto yF = heightF - heightF * binNorm;
					g.fillRect(xF, yF, 1.f, heightF - yF);
				}
			}
		}

		if (peakIndexes != nullptr)
		{
			std::function<float(int)> getBinIdxInverse;
			if(!logScaled)
				getBinIdxInverse = [&](int idx) { return static_cast<float>(idx); };
			else
				getBinIdxInverse = [&](int idx)
				{
					const auto xFreqRatio = static_cast<float>(idx) / widthF;
					if(xFreqRatio == 0.f)
						return 0.f;
					const auto xFreq = xFreqRatio * sampleRate;
					const auto xPitch = math::freqHzToNote2(xFreq);
					const auto xRatio = xPitch / maxPitch;
					const auto xF = xRatio * widthF;
					return xF;
				};

			g.setColour(juce::Colours::orange);
			const auto fontHeight = height * .01f;
			const auto fontHeight2 = static_cast<int>(fontHeight * 3.f);
			g.setFont(fontHeight);
			for (auto i = 0; i < numPeaks; ++i)
			{
				const auto idx = peakIndexes[i];
				const auto bin = static_cast<float>(bins[idx]);
				const auto absBin = std::abs(bin);
				const auto eps = std::numeric_limits<float>::epsilon();
				if (absBin > eps)
				{
					auto binDb = math::ampToDecibel(absBin);
					if (binDb > lowDb)
					{
						binDb = binDb > upDb ? upDb : binDb;
						const auto binNorm = (binDb - lowDb) / range;
						const auto yF = heightF - heightF * binNorm;
						const auto xF = getBinIdxInverse(idx);
						g.fillRect(xF, yF, 1.f, heightF - yF);
						const auto freqHz = idx * sampleRate / widthF;
						const auto txt = String(idx) + "\n" + String(freqHz) + " hz\n" + String(binDb) + "db";
						const auto x = static_cast<int>(xF) + 10;
						const auto y = static_cast<int>(yF);
						g.drawFittedText(txt, x, y, fontHeight2 * 4, fontHeight2, juce::Justification::left, 1);
					}
				}
			}
		}

		if (downsample && img.getWidth() > 1920)
		{
			const auto ratio = fftSize / 1920 + 1;
			img = img.rescaled(img.getWidth() / ratio, img.getHeight() / ratio, juce::Graphics::highResamplingQuality);
		}

		auto info = drawDebugText ? String(name + "\nmin: " + String(lowDb) + " db\nnum bins: " + String(fftSize)) : String();
		makeImage(g, img, name, info);
	}

	using Random = juce::Random;
}