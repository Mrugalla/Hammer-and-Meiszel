#include "ModalFilter.h"

namespace dsp
{
	namespace modal
	{
		// Material

		Material::Material() :
			buffer(),
			peakInfos(),
			updateState(UpdateState::Idle),
			sampleRate(0.f)
		{
		}

		void Material::load(const char* data, int size)
		{
			fillBuffer(data, size);
			load();
		}

		void Material::load()
		{
			std::vector<float> fifo, binsNorm;
			fifo.resize(FFTSize2);
			applyFFT(fifo.data());
			auto bins = fifo.data();
			{
				std::vector<float> binsLP;
				binsLP.resize(FFTSize2);
				applyLowpassIIR(binsLP.data(), bins, 50.f, .2f, 3);
				binsNorm.resize(FFTSize2);
				divide(binsNorm.data(), bins, binsLP.data());
				normalize(binsNorm.data());
			}
			const auto harm0Idx = getMaxMagnitudeIdx(binsNorm.data(), 0, FFTSizeHalf);
			std::vector<PeakIndexInfo> peakGroups;
			generatePeakGroups(peakGroups, binsNorm.data(), harm0Idx);
			std::vector<int> peakIndexes;
			peakIndexes.resize(NumFilters);
			generatePeakIndexes(peakIndexes, bins, peakGroups);
			generatePeakInfos(bins, peakIndexes.data(), static_cast<float>(harm0Idx));
			updateState.store(UpdateState::UpdateGUI);
		}

		void Material::fillBuffer(const char* data, int size)
		{
			AudioBufferF tmpBuffer;
			sampleRate = static_cast<float>(loadFromMemory(tmpBuffer, data, size));
			const auto samples = tmpBuffer.getArrayOfReadPointers();
			const auto numChannels = tmpBuffer.getNumChannels();
			const auto numSamples = tmpBuffer.getNumSamples();
			for (auto& b : buffer)
				b = 0.f;
			auto len = numSamples < FFTSize ? numSamples : FFTSize;
			for (auto s = 0; s < len; ++s)
				buffer[s] = samples[0][s];
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < len; ++s)
					buffer[s] += samples[ch][s];
			const auto gain = 1.f / static_cast<float>(numChannels);
			SIMD::multiply(buffer.data(), gain, len);
		}

		void Material::applyFFT(float* fifo)
		{
			FFT fft(FFTOrder);
			const auto windowFunc = [&](float x)
				{
					auto w = .5f - .5f * std::cos(TauF * x);
					return w;
				};
			for (auto i = 0; i < FFTSize; ++i)
			{
				const auto x = static_cast<float>(i) * FFTSizeInv;
				const auto wndw = windowFunc(x);
				fifo[i] = buffer[i] * wndw;
			}
			fft.performRealOnlyForwardTransform(fifo, true);
			generateMagnitudes(fifo);
			normalize(fifo);
		}

		void Material::generateMagnitudes(float* bins) noexcept
		{
			for (auto s = 0; s < FFTSize; ++s)
				bins[s] = std::abs(bins[s]);
		}

		void Material::normalize(float* bins) noexcept
		{
			auto max = 0.f;
			for (auto s = 0; s < FFTSize; ++s)
				if (max < bins[s])
					max = bins[s];
			const auto gain = 1.f / max;
			for (auto s = 0; s < FFTSize; ++s)
				bins[s] *= gain;
		}

		void Material::applyLowpassFIR(float* dest, const float* src,
			float cutoff, float bw)
		{
			auto irPtr = new ImpulseResponse<float, FFTSize>();
			auto& ir = *irPtr;
			ir.makeLowpass(44100.f, cutoff, bw, false);
			const auto lpLen = FFTSize + ir.getLatency();

			std::vector<int> wHead;
			wHead.reserve(lpLen);
			for (auto i = 0; i < lpLen; ++i)
				wHead.emplace_back(i);

			auto convolverPtr = new dsp::Convolver<float, FFTSize>(ir);
			auto& convolver = *convolverPtr;
			for (auto i = 0; i < lpLen; ++i)
				dest[i] = convolver.processSample(src[i], convolver.ringBuffer[0].data(), wHead[i]);
			applyNegativeDelay(dest, ir.getLatency());

			delete irPtr;
			delete convolverPtr;
		}

		void Material::applyLowpassIIR(float* dest, const float* src,
			float cutoff, float reso, int slope)
		{
			moog::BiquadFilter lp;
			lp.setType(moog::BiquadFilter::Type::LP);
			lp.setCutoffFc(cutoff / 44100.f);
			lp.setResonance(reso);
			lp.update();

			for (auto i = FFTSize - 1; i != 0; --i)
				dest[i] = static_cast<float>(lp(static_cast<double>(src[i])));
			for (auto i = 0; i < FFTSize; ++i)
				dest[i] = static_cast<float>(lp(static_cast<double>(dest[i])));

			for (auto i = 1; i < slope; ++i)
			{
				for (auto j = FFTSize - 1; j != 0; --j)
					dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
				for (auto j = 0; j < FFTSize; ++j)
					dest[j] = static_cast<float>(lp(static_cast<double>(dest[j])));
			}
		}

		void Material::applyNegativeDelay(float* bins, int offset) noexcept
		{
			for (auto s = 0; s < FFTSize; ++s)
				bins[s] = bins[(s + offset) % FFTSize];
		}

		void Material::divide(float* dest, const float* a, const float* b) noexcept
		{
			for (auto i = 0; i < FFTSize; ++i)
				if (b[i] != 0.f)
					dest[i] = a[i] / b[i];
				else
					dest[i] = 0.f;
		}

		int Material::getMaxMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) const noexcept
		{
			auto idx = lowerLimitIdx;
			auto max = bins[lowerLimitIdx];
			for (auto b = lowerLimitIdx; b < upperLimitIdx; ++b)
				if (max < bins[b])
				{
					max = bins[b];
					idx = b;
				}
			return idx;
		}

		int Material::getMinMagnitudeIdx(const float* bins, int lowerLimitIdx, int upperLimitIdx) const noexcept
		{
			auto idx = lowerLimitIdx;
			auto min = bins[lowerLimitIdx];
			for (auto b = lowerLimitIdx; b < upperLimitIdx; ++b)
				if (min > bins[b])
				{
					min = bins[b];
					idx = b;
				}
			return idx;
		}

		float Material::generatePeakGroups(std::vector<PeakIndexInfo>& peakGroups,
			const float* binsNorm, int minBinIdx)
		{
			auto targetThresholdDb = 0.f;
			do
			{
				targetThresholdDb -= 2.f;
				applyAdaptiveThreshold(peakGroups, binsNorm, targetThresholdDb, minBinIdx);
			} while (peakGroups.size() < NumFilters && targetThresholdDb > -60.f);

			if (peakGroups.size() < NumFilters)
			{
				PeakIndexInfo dummy;
				dummy.indexes.push_back(-1);
				peakGroups.resize(NumFilters, dummy);
			}

			return targetThresholdDb;
		}

		void Material::applyAdaptiveThreshold(std::vector<PeakIndexInfo>& peakGroups, const float* binsNorm,
			float targetThresholdDb, int minBinIdx)
		{
			const auto targetThreshold = math::dbToAmp(targetThresholdDb);
			peakGroups.clear();

			for (auto i = minBinIdx; i < FFTSize; ++i)
				if (binsNorm[i] > targetThreshold)
				{
					auto& peakInfo = peakGroups.emplace_back();
					peakInfo.indexes.push_back(i);

					const auto numNeighbors = 12;
					auto j = i;
					bool stillPeaking = false;
					do
					{
						stillPeaking = false;

						for (auto n = 0; n < numNeighbors; ++n)
						{
							++j;
							if (j >= FFTSize)
								return;
							if (binsNorm[j] > targetThreshold)
							{
								peakInfo.indexes.push_back(j);
								stillPeaking = true;
								n = numNeighbors;
							}
						}
					} while (stillPeaking);

					i = j + 1;
				}
		}

		void Material::generatePeakIndexes(std::vector<int>& peakIndexes, const float* bins,
			const std::vector<PeakIndexInfo>& peakIdxInfos)
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				auto& peakIndexInfo = peakIdxInfos[i];
				if (peakIndexInfo.indexes.front() != -1)
					peakIndexes[i] = getMaxMagnitudeIdx(bins, peakIndexInfo.indexes.front(), peakIndexInfo.indexes.back() + 1);
				else
					peakIndexes[i] = -1;
			}
		}

		float Material::getHarm0Idx(const float* fifo, const int* peakIndexes) const noexcept
		{
			auto peakIdx = peakIndexes[0];
			auto peakMag = fifo[peakIdx];
			auto harm0Mag = peakMag;
			auto harm0Idx = static_cast<float>(peakIdx);
			for (auto i = 1; i < NumFilters; ++i)
			{
				peakIdx = peakIndexes[i];
				peakMag = fifo[peakIdx];
				if (harm0Mag < peakMag)
				{
					harm0Mag = peakMag;
					harm0Idx = static_cast<float>(peakIdx);
				}
			}
			return harm0Idx;
		}

		int Material::getMinPeakIdx(const float* fifo, const int* peakIndexes) const noexcept
		{
			auto j = 0;
			auto peakIdx = peakIndexes[0];
			auto peakMag = fifo[peakIdx];
			for (auto i = 1; i < NumFilters; ++i)
			{
				const auto peakIdx1 = peakIndexes[i];
				const auto peakMag1 = fifo[peakIdx1];
				if (peakMag1 < peakMag)
				{
					peakMag = peakMag1;
					peakIdx = peakIdx1;
					j = i;
				}
			}
			return j;
		}

		void Material::generatePeakInfos(const float* bins, const int* peakIndexes, float harm0Idx) noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto peakIdx = peakIndexes[i];
				if (peakIdx != -1)
				{
					peakInfos[i].ratio = static_cast<double>(peakIdx) / static_cast<double>(harm0Idx);
					peakInfos[i].mag = static_cast<double>(bins[peakIdx]);
				}
				else
				{
					peakInfos[i].ratio = 1.;
					peakInfos[i].mag = 0.;
				}
			}
		}

		void Material::drawSpectrum(String&& name, const float* buf, float thresholdDb,
			const int* peakIndexes, bool isLog)
		{
			auto numFilters = 0;
			for(auto i = 0; i < NumFilters; ++i)
				numFilters += peakIndexes[i] != -1 ? 1 : 0;

			makeSpectrumImage(std::move(name), buf, FFTSize, thresholdDb - 6.f, false,
				peakIndexes, numFilters, true, isLog, sampleRate);
		}

		double calcBandwidthFc(double reso, double sampleRateInv) noexcept
		{
			const auto bw = 20. - math::tanhApprox(3. * reso) * 19.;
			const auto bwFc = bw * sampleRateInv;
			return bwFc;
		}

		void generateSine(Material& material)
		{
			material.sampleRate = static_cast<float>(material.buffer.size());
			for (auto s = 0; s < material.buffer.size(); ++s)
			{
				const auto sF = static_cast<float>(s);
				const auto x = sF / material.sampleRate;
				material.buffer[s] = std::cos(x * static_cast<float>(Pi));
			}
			material.load();
		}

		void generateSaw(Material& material)
		{
			material.sampleRate = static_cast<float>(material.buffer.size());
			for (auto s = 0; s < material.buffer.size(); ++s)
			{
				const auto sF = static_cast<float>(s);
				auto x = static_cast<float>(2 * NumFilters) * sF / material.sampleRate;
				x = x - std::floor(x);
				material.buffer[s] = 2.f * x - 1.f;
			}
			material.load();
		}

		void generateSquare(Material& material)
		{
			material.sampleRate = static_cast<float>(material.buffer.size());
			for (auto s = 0; s < material.buffer.size(); ++s)
			{
				const auto sF = static_cast<float>(s);
				auto x = static_cast<float>(2 * NumFilters) * sF / material.sampleRate;
				x = x - std::floor(x);
				material.buffer[s] = x < .5f ? 1.f : -1.f;
			}
			material.load();
		}

		// DualMaterial

		DualMaterial::DualMaterial() :
			materials(),
			peakInfos()
		{
			materials[0].load(BinaryData::Eiskaffee_wav, BinaryData::Eiskaffee_wavSize);
			//generateSine(materials[1]);
			//generateSquare(materials[0]);
			generateSaw(materials[1]);
			peakInfos = materials[0].peakInfos;
		}

		void DualMaterial::setMix(double mix, double harmonize, double saturate) noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto mag0 = materials[0].peakInfos[i].mag;
				const auto mag1 = materials[1].peakInfos[i].mag;
				const auto mag = mag0 + mix * (mag1 - mag0);
				peakInfos[i].mag = mag;

				const auto ratio0 = materials[0].peakInfos[i].ratio;
				const auto ratio1 = materials[1].peakInfos[i].ratio;
				const auto ratio = ratio0 + mix * (ratio1 - ratio0);

				peakInfos[i].ratio = ratio;
			}

			if (harmonize != 0.)
				for (auto i = 0; i < NumFilters; ++i)
					peakInfos[i].ratio += harmonize * (std::round(peakInfos[i].ratio) - peakInfos[i].ratio);

			if (saturate != 0.)
			{
				const auto p = (.999 * saturate + 1.) * .5;
				for (auto i = 0; i < NumFilters; ++i)
				{
					const auto x = peakInfos[i].mag;
					const auto y = p * x / ((1. - p) - x + 2. * p * x);
					peakInfos[i].mag = y;
				}
			}

			for(auto i = 0; i < 2; ++i)
				if(materials[i].updateState.load() == Material::UpdateState::UpdateProcessor)
					materials[i].updateState.store(Material::UpdateState::UpdateGUI);
		}

		double DualMaterial::getMag(int i) const noexcept
		{
			return peakInfos[i].mag;
		}

		double DualMaterial::getRatio(int i) const noexcept
		{
			return peakInfos[i].ratio;
		}

		bool DualMaterial::wantsUpdate() const noexcept
		{
			return materials[0].updateState.load() == Material::UpdateState::UpdateProcessor ||
				materials[1].updateState.load() == Material::UpdateState::UpdateProcessor;
		}

		// Envelope

		Envelope::Envelope() :
			env(0.),
			atk(0.),
			rls(0.),
			state(State::Release),
			noteOn(false)
		{}

		double Envelope::operator()(bool _noteOn) noexcept
		{
			noteOn = _noteOn;
			return operator()();
		}

		double Envelope::operator()() noexcept
		{
			switch (state)
			{
			case State::Attack: return processAttack();
			case State::Release: return processRelease();
			default: return env;
			}
		}

		double Envelope::processAttack() noexcept
		{
			if (noteOn)
			{
				env += atk * (1. - env);
				if (1. - env < 1e-6)
					env = 1.;
				return env;
			}
			state = State::Release;
			return processRelease();
		}

		double Envelope::processRelease() noexcept
		{
			if (noteOn)
			{
				state = State::Attack;
				return processAttack();
			}
			env += rls * (0. - env);
			if (env < 1e-6)
				env = 0.;
			return env;
		}

		// Resonator

		Resonator::Val::Val() :
			pitch(0.),
			transpose(0.),
			pb(0.),
			pbRange(2.),
			xen(12.)
		{}

		double Resonator::Val::getFreq(const arch::XenManager& xen) noexcept
		{
			return xen.noteToFreqHzWithWrap(pitch + transpose + pb * pbRange * 2.);
		}

		Resonator::Resonator(const arch::XenManager& _xen, const DualMaterial& _material) :
			xen(_xen),
			material(_material),
			resonators(),
			val(),
			freqHz(1000.),
			sampleRate(1.),
			nyquist(.5),
			numFiltersBelowNyquist(0)
		{
		}

		void Resonator::reset() noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
				resonators[i].reset();
		}

		void Resonator::prepare(double _sampleRate)
		{
			sampleRate = _sampleRate;
			nyquist = sampleRate * .5;
			setFrequencyHz(1000.);
		}

		void Resonator::operator()(double** samples, const MidiBuffer& midi,
			double transposeSemi, int numChannels, int numSamples) noexcept
		{
			if (val.transpose != transposeSemi ||
				val.pbRange != xen.getPitchbendRange() ||
				val.xen != xen.getXen())
			{
				val.transpose = transposeSemi;
				val.pbRange = xen.getPitchbendRange();
				val.xen = xen.getXen();
				const auto freq = val.getFreq(xen);
				setFrequencyHz(freq);
			}

			process(samples, midi, numChannels, numSamples);
		}

		void Resonator::setFrequencyHz(double freq) noexcept
		{
			if (freqHz == freq)
				return;
			freqHz = freq;
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto freqRatio = material.getRatio(i);
				const auto freqFilter = freqHz * freqRatio;
				if (freqFilter < nyquist)
				{
					const auto fc = math::freqHzToFc(freqFilter, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc);
					//resonator.reset();
					resonator.update();
				}
				else
				{
					numFiltersBelowNyquist = i;
					return;
				}
			}
			numFiltersBelowNyquist = NumFilters;
		}

		void Resonator::updateFreqRatios() noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				const auto freqRatio = material.getRatio(i);
				const auto freqFilter = freqHz * freqRatio;
				if (freqFilter < nyquist)
				{
					const auto fc = math::freqHzToFc(freqFilter, sampleRate);
					auto& resonator = resonators[i];
					resonator.setCutoffFc(fc);
					resonator.update();
				}
				else
				{
					numFiltersBelowNyquist = i;
					return;
				}
			}
			numFiltersBelowNyquist = NumFilters;
		}

		void Resonator::setBandwidth(double bw) noexcept
		{
			for (auto i = 0; i < NumFilters; ++i)
			{
				auto& resonator = resonators[i];
				resonator.setBandwidth(bw);
				resonator.update();
			}
		}

		void Resonator::process(double** samples, const MidiBuffer& midi, int numChannels, int numSamples) noexcept
		{
			static constexpr double PB = 0x3fff;
			static constexpr double PBInv = 1. / PB;

			auto s = 0;
			for (const auto it : midi)
			{
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					const auto ts = it.samplePosition;
					val.pitch = static_cast<double>(msg.getNoteNumber());
					const auto freq = val.getFreq(xen);
					setFrequencyHz(freq);
					applyFilter(samples, numChannels, s, ts);
					s = ts;
				}
				else if (msg.isPitchWheel())
				{
					const auto ts = it.samplePosition;
					const auto pb = static_cast<double>(msg.getPitchWheelValue()) * PBInv - .5;
					if (val.pb != pb)
					{
						val.pb = pb;
						const auto freq = val.getFreq(xen);
						setFrequencyHz(freq);
						applyFilter(samples, numChannels, s, ts);
						s = ts;
					}
				}
			}

			applyFilter(samples, numChannels, s, numSamples);
		}

		void Resonator::applyFilter(double** samples, int numChannels,
			int startIdx, int endIdx) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				for (auto i = startIdx; i < endIdx; ++i)
				{
					const auto dry = smpls[i];
					auto wet = 0.;
					for (auto f = 0; f < numFiltersBelowNyquist; ++f)
						wet += resonators[f](dry, ch) * material.getMag(f);
					smpls[i] = wet;
				}
			}
		}

		// Voice

		Voice::Voice(const arch::XenManager& xen, const DualMaterial& material) :
			filter(xen, material),
			env(),
			envBuffer(),
			gain(1.)
		{}

		void Voice::prepare(double sampleRate)
		{
			filter.prepare(sampleRate);
			env.atk = math::msToInc(2., sampleRate);
			env.rls = math::msToInc(10., sampleRate);
		}

		void Voice::setBandwidth(double bw) noexcept
		{
			filter.setBandwidth(bw);
		}

		void Voice::operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
			double transposeSemi, int numChannels, int numSamples) noexcept
		{
			synthesizeEnvelope(midi, numSamples);
			processEnvelope(samplesSrc, samplesDest, numChannels, numSamples);
			filter(samplesDest, midi, transposeSemi, numChannels, numSamples);
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
			if (env.state != Envelope::State::Release)
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

		// Filter

		Filter::Filter(const arch::XenManager& xen) :
			autoGainReso(),
			material(),
			voices
			{
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material),
				Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material), Voice(xen, material)
			},
			modalMix(-1.),
			modalHarmonize(-1.),
			modalSaturate(-1.),
			reso(-1.),
			sampleRateInv(1.)
		{
			initAutoGainReso();
		}

		void Filter::prepare(double sampleRate)
		{
			for (auto& voice : voices)
				voice.prepare(sampleRate);
			reso = -1.;
			sampleRateInv = 1. / sampleRate;
		}

		void Filter::updateParameters(double _modalMix, double _modalHarmonize,
			double _modalSaturate, double _reso) noexcept
		{
			updateModalMix(_modalMix, _modalHarmonize, _modalSaturate);
			updateReso(_reso);
		}

		void Filter::operator()(const double** samplesSrc, double** samplesDest, const MidiBuffer& midi,
			double transposeSemi, int numChannels, int numSamples, int v) noexcept
		{
			voices[v](samplesSrc, samplesDest, midi, transposeSemi, numChannels, numSamples);
		}

		void Filter::updateModalMix(double _modalMix, double _modalHarmonize, double _modalSaturate) noexcept
		{
			bool wantsUpdate = material.wantsUpdate();
			if (modalMix == _modalMix && modalHarmonize == _modalHarmonize && modalSaturate == _modalSaturate && !wantsUpdate)
				return;
			modalMix = _modalMix;
			modalHarmonize = _modalHarmonize;
			modalSaturate = _modalSaturate;
			material.setMix(modalMix, modalHarmonize, modalSaturate);
			for (auto& voice : voices)
				voice.filter.updateFreqRatios();
		}

		void Filter::updateReso(double _reso) noexcept
		{
			if (reso == _reso)
				return;
			reso = _reso;
			autoGainReso.updateParameterValue(reso);
			const auto bwFc = calcBandwidthFc(reso, sampleRateInv);
			for (auto& voice : voices)
			{
				voice.setBandwidth(bwFc);
				voice.gain = autoGainReso();
			}
		}

		void Filter::initAutoGainReso()
		{
			Resonator2 resonator;
			resonator.setCutoffFc(500. / 44100.);

			autoGainReso.prepareGains
			(
				[&](double* smpls, double _reso, int numSamples)
				{
					reso = _reso;
					const auto bwFc = calcBandwidthFc(reso, 1. / 44100.);
					resonator.setBandwidth(bwFc);
					resonator.update();
					resonator.reset();
					for (auto s = 0; s < numSamples; ++s)
					{
						const auto dry = smpls[s];
						const auto wet = resonator(dry);
						smpls[s] = wet;
					}
				}
			);
		}
	}
}