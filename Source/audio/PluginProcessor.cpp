#include "PluginProcessor.h"
#include "../arch/Math.h"
//#include "../HammerUndMeiszelTests.h"

namespace audio
{
	PluginProcessor::PluginProcessor(Params& _params, const arch::XenManager& _xen) :
		params(_params),
		xen(_xen),
		sampleRate(1.),
		autoMPE(),
		voiceSplit(),
		parallelProcessor(),
		modalFilter(xen)
	{
		//test::SpeedTestPB([&](double** samples, dsp::MidiBuffer& midi, int numChannels, int numSamples)
		//{
		//	prepare(44100.);
		//	operator()(samples, midi, 2, dsp::BlockSize2x);
		//}, 10, "i am speed");
	}

	void PluginProcessor::prepare(double _sampleRate)
	{
		sampleRate = _sampleRate;
		modalFilter.prepare(sampleRate);
	}

	void PluginProcessor::operator()(double** samples, dsp::MidiBuffer& midi, int numChannels, int numSamples) noexcept
	{
		//randomMelodyGenerator(midi, numSamples);

		autoMPE(midi); voiceSplit(midi);

		const auto samplesConst = const_cast<const double**>(samples);

		const auto& resoParam = params(PID::Resonance);
		const auto reso = static_cast<double>(resoParam.getValMod());

		modalFilter(samplesConst, voiceSplit, parallelProcessor, reso, numChannels, numSamples);
		parallelProcessor.joinReplace(samples, numChannels, numSamples);
	}

	void PluginProcessor::processBlockBypassed(double**, dsp::MidiBuffer&, int, int) noexcept
	{}

	void PluginProcessor::savePatch()
	{}

	void PluginProcessor::loadPatch()
	{}
}