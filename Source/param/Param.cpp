#include "Param.h"
#include "../arch/FormulaParser.h"
#include "../arch/Range.h"

namespace param
{
	String toID(const String& name)
	{
		return name.removeCharacters(" ").toLowerCase();
	}

	PID ll(PID pID, int off) noexcept
	{
		auto i = static_cast<int>(pID);
		i += (NumLowLevelParams - 1) * off;
		return static_cast<PID>(i);
	}

	PID offset(PID pID, int off) noexcept
	{
		return static_cast<PID>(static_cast<int>(pID) + off);
	}

	String toString(PID pID)
	{
		switch (pID)
		{
		case PID::Macro: return "Macro";
#if PPDIsNonlinear
		case PID::GainIn: return "Gain In";
		case PID::UnityGain: return "Unity Gain";
#endif
#if PPDIO == PPDIODryWet
		case PID::GainDry: return "Gain Dry";
		case PID::GainWet: return "Gain Wet";
#elif PPDIO == PPDIOWetMix
		case PID::GainWet: return "Gain Wet";
		case PID::Mix: return "Mix";
#if PPDHasDelta
		case PID::Delta: return "Delta";
#endif
#endif
		case PID::GainOut: return "Gain Out";
#if PPDHasStereoConfig
		case PID::StereoConfig: return "Stereo Config";
#endif
#if PPDHasHQ
		case PID::HQ: return "HQ";
#endif
#if PPDHasLookahead
		case PID::Lookahead: return "Lookahead";
#endif
#if PPDHasTuningEditor
			// TUNING PARAM:
		case PID::Xen: return "Xen";
		case PID::XenSnap: return "Xen Snap";
		case PID::MasterTune: return "Master Tune";
		case PID::AnchorPitch: return "Anchor Pitch";
		case PID::PitchbendRange: return "Pitchbend Range";
#endif
		case PID::SoftClip: return "Soft Clip";
		case PID::Power: return "Power";

		// LOW LEVEL PARAMS:
		case PID::NoiseBlend: return "Noise Blend";
		case PID::KeySelectorEnabled: return "Key Selector Enabled";
		//
		case PID::EnvGenAmpAttack: return "EnvGenAmp Attack";
		case PID::EnvGenAmpDecay: return "EnvGenAmp Decay";
		case PID::EnvGenAmpSustain: return "EnvGenAmp Sustain";
		case PID::EnvGenAmpRelease: return "EnvGenAmp Release";
		//
		case PID::ModSelect: return "Mod Select";
		//
		case PID::EnvGenModAttack: return "EnvGenMod Attack";
		case PID::EnvGenModDecay: return "EnvGenMod Decay";
		case PID::EnvGenModSustain: return "EnvGenMod Sustain";
		case PID::EnvGenModRelease: return "EnvGenMod Release";
		case PID::EnvGenModAttackTS: return "EnvGenMod Attack Sync";
		case PID::EnvGenModDecayTS: return "EnvGenMod Decay Sync";
		case PID::EnvGenModReleaseTS: return "EnvGenMod Release Sync";
		case PID::EnvGenModTemposync: return "EnvGenMod Temposync";
		//
		case PID::EnvFolModGain: return "EnvFolMod Gain";
		case PID::EnvFolModAttack: return "EnvFolMod Attack";
		case PID::EnvFolModDecay: return "EnvFolMod Decay";
		case PID::EnvFolModSmooth: return "EnvFolMod Smooth";
		//
		case PID::RandModRateSync: return "RandMod Rate Sync";
		case PID::RandModSmooth: return "RandMod Smooth";
		case PID::RandModComplex: return "RandMod Complex";
		case PID::RandModDropout: return "RandMod Dropout";
		//
		case PID::ModalOct: return "Modal Oct";
		case PID::ModalSemi: return "Modal Semi";
		case PID::ModalBlend: return "Modal Blend";
		case PID::ModalBlendEnv: return "Modal Blend Env";
		case PID::ModalBlendBreite: return "Modal Blend Breite";
		case PID::ModalSpreizung: return "Modal Spreizung";
		case PID::ModalSpreizungEnv: return "Modal Spreizung Env";
		case PID::ModalSpreizungBreite: return "Modal Spreizung Breite";
		case PID::ModalHarmonie: return "Modal Harmonie";
		case PID::ModalHarmonieEnv: return "Modal Harmonie Env";
		case PID::ModalHarmonieBreite: return "Modal Harmonie Breite";
		case PID::ModalKraft: return "Modal Kraft";
		case PID::ModalKraftEnv: return "Modal Kraft Env";
		case PID::ModalKraftBreite: return "Modal Kraft Breite";
		case PID::ModalResonanz: return "Modal Reso";
		case PID::ModalResonanzEnv: return "Modal Reso Env";
		case PID::ModalResonanzBreite: return "Modal Reso Breite";
		//
		case PID::Polyphony: return "Polyphony";
		//
		case PID::FormantA: return "Formant A";
		case PID::FormantB: return "Formant B";
		case PID::FormantPos: return "Formant Position";
		case PID::FormantPosEnv: return "Formant Position Env";
		case PID::FormantPosWidth: return "Formant Position Width";
		case PID::FormantQ: return "Formant Q";
		case PID::FormantQEnv: return "Formant Q Env";
		case PID::FormantQWidth: return "Formant Q Width";
		case PID::FormantDecay: return "Formant Decay";
		case PID::FormantGain: return "Formant Gain";
		//
		case PID::CombOct: return "Comb Oct";
		case PID::CombSemi: return "Comb Semi";
		case PID::CombUnison: return "Comb Unison";
		case PID::CombFeedback: return "Comb Feedback";
		case PID::CombFeedbackEnv: return "Comb Feedback Env";
		case PID::CombFeedbackWidth: return "Comb Feedback Width";
		//
		case PID::Damp: return "Damp";
		case PID::DampEnv: return "Damp Env";
		case PID::DampWidth: return "Damp Width";
		//
		default: return "Invalid Parameter Name";
		}
	}

	PID toPID(const String& id)
	{
		const auto nID = toID(id);
		for (auto i = 0; i < NumParams; ++i)
		{
			auto pID = static_cast<PID>(i);
			if (nID == toID(toString(pID)))
				return pID;
		}
		return PID::NumParams;
	}

	void toPIDs(std::vector<PID>& pIDs, const String& text, const String& seperator)
	{
		auto tokens = juce::StringArray::fromTokens(text, seperator, "");
		for (auto& token : tokens)
		{
			auto pID = toPID(token);
			if (pID != PID::NumParams)
				pIDs.push_back(pID);
		}
	}

	String toTooltip(PID pID)
	{
		switch (pID)
		{
		case PID::Macro: return "Dial in the desired amount of macro modulation depth.";
#if PPDIsNonlinear
		case PID::GainIn: return "Apply gain to the input signal.";
		case PID::UnityGain: return "Compensates for the added input gain.";
#endif
#if PPDIO == PPDIODryWet
		case PID::GainDry: return "Apply gain to the dry signal.";
		case PID::GainWet: return "Apply gain to the wet signal.";
#elif PPDIO == PPDIOWetMix
		case PID::GainWet: return "Apply gain to the wet signal.";
		case PID::Mix: return "Mix the dry with the wet signal.";
#if PPDHasDelta
		case PID::Delta: return "Listen to the difference between the dry and the wet signal.";
#endif
#endif
		case PID::GainOut: return "Apply gain to the output signal.";
#if PPDHasStereoConfig
		case PID::StereoConfig: return "Switch between L/R and M/S mode.";
#endif
#if PPDHasHQ
		case PID::HQ: return "Apply oversampling to the signal.";
#endif

#if PPDHasLookahead
		case PID::Lookahead: return "Dis/Enabled lookahead.";
#endif
#if PPDHasTuningEditor
		// TUNING PARAMS:
		case PID::Xen: return "The xenharmony describes how many pitches per octave exist. Higher edo = smaller intervals.";
		case PID::XenSnap: return "If disabled you can explore xen scales that have less or no octaves.";
		case PID::MasterTune: return "This is the frequency the anchor pitch refers to. (Chamber Frequency)";
		case PID::AnchorPitch: return "The anchor pitch refers to the same frequency regardless of xen scale.";
		case PID::PitchbendRange: return "The pitchbend range in semitones describes how many pitches you can bend.";
#endif
		case PID::SoftClip: return "Dis/Enable soft clipping.";
		case PID::Power: return "Dis/Enable the plugin.";

		// LOW LEVEL PARAMS:
		case PID::NoiseBlend: return "Blends between the dry signal and the noise synth.";
		case PID::KeySelectorEnabled: return "If enabled the modal filters are triggered by the key selector.";
		//
		case PID::EnvGenAmpAttack: return "The amplitude envelope generator's attack time.";
		case PID::EnvGenAmpDecay: return "The amplitude envelope generator's decay time.";
		case PID::EnvGenAmpSustain: return "The amplitude envelope generator's sustain level.";
		case PID::EnvGenAmpRelease: return "The amplitude envelope generator's release time.";
		//
		case PID::ModSelect: return "Select the modulation source.";
		//
		case PID::EnvGenModAttack: return "The modulation envelope generator's attack time in secs/ms.";
		case PID::EnvGenModDecay: return "The modulation envelope generator's decay time in secs/ms.";
		case PID::EnvGenModSustain: return "The modulation envelope generator's sustain level.";
		case PID::EnvGenModRelease: return "The modulation envelope generator's release time in secs/ms.";
		case PID::EnvGenModAttackTS: return "The modulation envelope generator's attack time in sync with the tempo.";
		case PID::EnvGenModDecayTS: return "The modulation envelope generator's decay time in sync with the tempo.";
		case PID::EnvGenModReleaseTS: return "The modulation envelope generator's release time in sync with the tempo.";
		case PID::EnvGenModTemposync: return "Dis/Enable temposync for the modulation envelope generator.";
		//
		case PID::EnvFolModGain: return "Gain stage the envelope follower with this control.";
		case PID::EnvFolModAttack: return "The envelope follower's attack time in ms.";
		case PID::EnvFolModDecay: return "The envelope follower's decay time in ms.";
		case PID::EnvFolModSmooth: return "Smoothen the envelope follower's output with this control.";
		//
		case PID::RandModRateSync: return "The rate of the randomizer in sync with the tempo.";
		case PID::RandModSmooth: return "Blends between steppy and smooth modulation.";
		case PID::RandModComplex: return "Adds more octaves of quieter noise to the modulation.";
		case PID::RandModDropout: return "Like a touge with lots of technical sections.";
		//
		case PID::ModalOct: return "Transposes the modal fitler in octaves.";
		case PID::ModalSemi: return "Transposes the modal fitler in semitones.";
		case PID::ModalBlend: return "Blends between the 2 modal filters.";
		case PID::ModalBlendEnv: return "The envelope generator's depth on the modal blend.";
		case PID::ModalBlendBreite: return "The stereo width of the modal breite.";
		case PID::ModalSpreizung: return "Spreads or shrinks the resonators' frequency ratios.";
		case PID::ModalSpreizungEnv: return "The envelope generator's depth on the modal spreizung.";
		case PID::ModalSpreizungBreite: return "The stereo width of the modal spreizung.";
		case PID::ModalHarmonie: return "Harmonizes the resonators' frequency ratios towards the harmonic series.";
		case PID::ModalHarmonieEnv: return "The envelope generator's depth on the modal harmonie.";
		case PID::ModalHarmonieBreite: return "The stereo width of the modal harmonie.";
		case PID::ModalKraft: return "Saturates the resonators' magnitude values.";
		case PID::ModalKraftEnv: return "The envelope generator's depth on the modal kraft.";
		case PID::ModalKraftBreite: return "The stereo width of the modal kraft.";
		case PID::ModalResonanz: return "Higher resonance causes sharper ringing.";
		case PID::ModalResonanzEnv: return "The envelope generator's depth on the modal resonanz.";
		case PID::ModalResonanzBreite: return "The stereo width of the modal resonanz.";
		//
		case PID::Polyphony: return "The polyphony (number of voices) of the synth engine.";
		//
		case PID::FormantA: return "Vowel A of the formant filter.";
		case PID::FormantB: return "Vowel B of the formant filter.";
		case PID::FormantPos: return "The position between the selected vowel";
		case PID::FormantPosEnv: return "The envelope generator's depth on the formant position.";
		case PID::FormantPosWidth: return "The stereo width of the formant position.";
		case PID::FormantQ: return "The formant filter's resonance.";
		case PID::FormantQEnv: return "The envelope generator's depth on the formant filter's resonance.";
		case PID::FormantQWidth: return "The stereo width of the formant filter's resonance.";
		case PID::FormantDecay: return "The formant filter's decay time.";
		case PID::FormantGain: return "The formant filter's output gain.";
		//
		case PID::CombOct: return "Transposes the comb filter in octaves.";
		case PID::CombSemi: return "Transposes the comb filter in semitones.";
		case PID::CombUnison: return "Applies unison of maximally 1 semitone to the comb filter.";
		case PID::CombFeedback: return "The feedback of the comb filter's feedback delay.";
		case PID::CombFeedbackEnv: return "The envelope generator's depth on the comb filter's feedback.";
		case PID::CombFeedbackWidth: return "The stereo width of the comb filter's feedback.";
		//
		case PID::Damp: return "Dampens the voice with a lowpass filter.";
		case PID::DampEnv: return "The envelope generator's depth on the dampening.";
		case PID::DampWidth: return "The stereo width of the dampening.";
		//
		default: return "Invalid Tooltip.";
		}
	}

	String toString(Unit pID)
	{
		switch (pID)
		{
		case Unit::Power: return "";
		case Unit::Solo: return "S";
		case Unit::Mute: return "M";
		case Unit::Percent: return "%";
		case Unit::Hz: return "hz";
		case Unit::Beats: return "";
		case Unit::Degree: return CharPtr("\xc2\xb0");
		case Unit::Octaves: return "oct";
		case Unit::Semi: return "semi";
		case Unit::Fine: return "ct";
		case Unit::Ms: return "ms";
		case Unit::Decibel: return "db";
		case Unit::Ratio: return "ratio";
		case Unit::Polarity: return CharPtr("\xc2\xb0");
		case Unit::StereoConfig: return "";
		case Unit::Voices: return "v";
		case Unit::Pan: return "%";
		case Unit::Xen: return "notes/oct";
		case Unit::Note: return "";
		case Unit::Pitch: return "";
		case Unit::Q: return "q";
		case Unit::Slope: return "db/oct";
		case Unit::Legato: return "";
		case Unit::Custom: return "";
		case Unit::FilterType: return "";
		default: return "";
		}
	}

	// PARAM:

	Param::Mod::Mod() :
		depth(0.f),
		bias(.5f)
	{}

	Param::Param(const PID pID, const Range& _range, const float _valDenormDefault,
		const ValToStrFunc& _valToStr, const StrToValFunc& _strToVal, const Unit _unit) :
		AudioProcessorParameter(),
		id(pID),
		range(_range),
		valDenormDefault(range.snapToLegalValue(_valDenormDefault)),
		valInternal(range.convertTo0to1(valDenormDefault)),
		mod(),
		valNorm(valInternal), valMod(valNorm.load()),
		valToStr(_valToStr), strToVal(_strToVal), unit(_unit),
		locked(false), inGesture(false), modDepthAbsolute(false)
	{
	}

	void Param::savePatch(State& state) const
	{
		const String idStr("params/" + toID(toString(id)));
		const auto v = range.convertFrom0to1(getValue());
		state.set(idStr + "/value", v);
		const auto md = getModDepth();
		state.set(idStr + "/md", md);
		const auto mb = getModBias();
		state.set(idStr + "/mb", mb);
	}

	void Param::loadPatch(const State& state)
	{
		if (isLocked())
			return;
		const String idStr("params/" + toID(toString(id)));
		auto var = state.get(idStr + "/value");
		if (var)
		{
			const auto val = static_cast<float>(*var);
			const auto legalVal = range.snapToLegalValue(val);
			const auto valD = range.convertTo0to1(legalVal);
			setValueNotifyingHost(valD);
		}
		var = state.get(idStr + "/md");
		if (var)
		{
			const auto val = static_cast<float>(*var);
			setModDepth(val);
		}
		var = state.get(idStr + "/mb");
		if (var)
		{
			const auto val = static_cast<float>(*var);
			setModBias(val);
		}
	}

	//called by host, normalized, thread-safe
	float Param::getValue() const
	{
		return valNorm.load();
	}

	float Param::getValueDenorm() const noexcept
	{
		return range.convertFrom0to1(getValue());
	}

	void Param::setValue(float normalized)
	{
		if (isLocked())
			return;

		if (!modDepthAbsolute)
			return valNorm.store(normalized);

		const auto pLast = valNorm.load();
		const auto pCur = normalized;

		const auto dLast = getModDepth();
		const auto dCur = dLast - pCur + pLast;
		setModDepth(dCur);

		valNorm.store(pCur);
	}

	void Param::setValueFromEditor(float x) noexcept
	{
		const auto lckd = isLocked();
		setLocked(false);
		setValueNotifyingHost(x);
		setLocked(lckd);
	}

	// called by editor
	bool Param::isInGesture() const noexcept
	{
		return inGesture.load();
	}

	// called by editor
	void Param::setValueWithGesture(float norm)
	{
		if (isInGesture())
			return;
		beginChangeGesture();
		setValueFromEditor(norm);
		endChangeGesture();
	}

	void Param::beginGesture()
	{
		inGesture.store(true);
		beginChangeGesture();
	}

	void Param::endGesture()
	{
		inGesture.store(false);
		endChangeGesture();
	}

	float Param::getModDepth() const noexcept
	{
		return mod.depth.load();
	}

	void Param::setModDepth(float v) noexcept
	{
		if (isLocked())
			return;

		mod.depth.store(juce::jlimit(-1.f, 1.f, v));
	}

	float Param::calcValModOf(float modSrc) const noexcept
	{
		const auto md = mod.depth.load();
		const auto pol = md > 0.f ? 1.f : -1.f;
		const auto dAbs = md * pol;
		const auto dRemapped = biased(0.f, dAbs, mod.bias.load(), modSrc);
		const auto mValue = dRemapped * pol;

		return mValue;
	}

	float Param::getValMod() const noexcept
	{
		return valMod.load();
	}

	float Param::getValModDenorm() const noexcept
	{
		return range.convertFrom0to1(valMod.load());
	}

	void Param::setModBias(float b) noexcept
	{
		if (isLocked())
			return;
		b = BiasEps + b * (1.f - 2.f * BiasEps);
		b = juce::jlimit(BiasEps, 1.f - BiasEps, b);
		mod.bias.store(b);
	}

	float Param::getModBias() const noexcept
	{
		return mod.bias.load();
	}

	void Param::setModulationDefault() noexcept
	{
		setModDepth(0.f);
		setModBias(.5f);
	}

	void Param::setModDepthAbsolute(bool e) noexcept
	{
		modDepthAbsolute = e;
	}

	void Param::setDefaultValue(float norm) noexcept
	{
		valDenormDefault = range.convertFrom0to1(norm);
	}

	void Param::startModulation() noexcept
	{
		valInternal = getValue();
	}

	void Param::modulate(float modSrc) noexcept
	{
		valInternal += calcValModOf(modSrc);
	}

	void Param::endModulation() noexcept
	{
		valMod.store(juce::jlimit(0.f, 1.f, valInternal));
	}

	float Param::getDefaultValue() const
	{
		return range.convertTo0to1(valDenormDefault);
	}

	String Param::getName(int) const
	{
		return toString(id);
	}

	// units of param (hz, % etc.)
	String Param::getLabel() const
	{
		return toString(unit);
	}

	// string of norm val
	String Param::getText(float norm, int) const
	{
		return valToStr(range.snapToLegalValue(range.convertFrom0to1(norm)));
	}

	// string to norm val
	float Param::getValueForText(const String& text) const
	{
		const auto val = juce::jlimit(range.start, range.end, strToVal(text));
		return range.convertTo0to1(val);
	}

	// string to denorm val
	float Param::getValForTextDenorm(const String& text) const
	{
		return strToVal(text);
	}

	String Param::_toString()
	{
		auto v = getValue();
		return getName(10) + ": " + String(v) + "; " + getText(v, 10);
	}

	int Param::getNumSteps() const
	{
		if (range.interval > 0.f)
		{
			const auto numSteps = (range.end - range.start) / range.interval;
			return 1 + static_cast<int>(numSteps);
		}

		return juce::AudioProcessor::getDefaultNumParameterSteps();
	}

	bool Param::isLocked() const noexcept
	{
		return locked.load();
	}

	void Param::setLocked(bool e) noexcept
	{
		locked.store(e);
	}

	void Param::switchLock() noexcept
	{
		setLocked(!isLocked());
	}

	float Param::biased(float start, float end, float bias, float x) noexcept
	{
		const auto r = end - start;
		if (r == 0.f)
			return 0.f;
		const auto a2 = 2.f * bias;
		const auto aM = 1.f - bias;
		const auto aR = r * bias;
		return start + aR * x / (aM - x + a2 * x);
	}
}

namespace param::strToVal
{
	extern std::function<float(String, const float)> parse()
	{
		return [](const String& txt, const float altVal)
		{
			fx::Parser fx;
			if (fx(txt))
				return fx();
			return altVal;
		};
	}

	StrToValFunc standard(float defaultVal)
	{
		return[p = parse(), defaultVal](const String& txt)
		{
			if (math::stringNegates(txt))
				return defaultVal;
			const auto val = p(txt, defaultVal);
			return val;
		};
	}

	StrToValFunc power()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Power));
			if (math::stringNegates(text))
				return 0.f;
			const auto val = p(text, 0.f);
			return val > .5f ? 1.f : 0.f;
		};
	}

	StrToValFunc solo()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Solo));
			const auto val = p(text, 0.f);
			return val > .5f ? 1.f : 0.f;
		};
	}

	StrToValFunc mute()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Mute));
			const auto val = p(text, 0.f);
			return val > .5f ? 1.f : 0.f;
		};
	}

	StrToValFunc percent()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Percent));
			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc hz()
	{
		return[p = parse()](const String& txt)
		{
			auto text = txt.trimCharactersAtEnd(toString(Unit::Hz));
			auto multiplier = 1.f;
			if (text.getLastCharacter() == 'k')
			{
				multiplier = 1000.f;
				text = text.dropLastCharacters(1);
			}
			const auto val = p(text, 0.f);
			const auto val2 = val * multiplier;

			return val2;
		};
	}

	StrToValFunc phase()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Degree));
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc oct()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Octaves));
			const auto val = p(text, 0.f);
			return std::round(val);
		};
	}

	StrToValFunc octFloat()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Octaves));
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc semi()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Semi));
			const auto val = p(text, 0.f);
			return std::round(val);
		};
	}

	StrToValFunc fine()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Fine));
			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc ratio()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Ratio));
			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc lrms()
	{
		return [](const String& txt)
		{
			return txt[0] == 'l' ? 0.f : 1.f;
		};
	}

	StrToValFunc freeSync()
	{
		return [](const String& txt)
		{
			return txt[0] == 'f' ? 0.f : 1.f;
		};
	}

	StrToValFunc polarity()
	{
		return [](const String& txt)
		{
			return txt[0] == '0' ? 0.f : 1.f;
		};
	}

	StrToValFunc ms()
	{
		return[p = parse()](const String& txt)
		{
			auto iStr = 0;
			for(auto i = 0; i < txt.length(); ++i)
				if (txt[i] >= 'a' && txt[i] <= 'z' || txt[i] >= 'A' && txt[i] <= 'Z')
				{
					iStr = i;
					i = txt.length();
				}
			const auto text = txt.substring(0, iStr);
			const auto val = p(text, 0.f);
			if(txt.endsWith("sec") || txt.endsWith("sek"))
				return val * 1000.f;
			else
				return val;
		};
	}

	StrToValFunc db()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Decibel));
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc voices()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Voices));
			const auto val = p(text, 1.f);
			return val;
		};
	}

	StrToValFunc pan(const Params& params)
	{
		return[p = parse(), &prms = params](const String& txt)
		{
			if (txt == "center" || txt == "centre")
				return 0.f;

			const auto text = txt.trimCharactersAtEnd("MSLR").toLowerCase();
#if PPDHasStereoConfig
			const auto sc = prms[PID::StereoConfig];
			if (sc->getValMod() < .5f)
#endif
			{
				if (txt == "l" || txt == "left")
					return -1.f;
				else if (txt == "r" || txt == "right")
					return 1.f;
			}
#if PPDHasStereoConfig
			else
			{

				if (txt == "m" || txt == "mid")
					return -1.f;
				else if (txt == "s" || txt == "side")
					return 1.f;
			}
#endif

			const auto val = p(text, 0.f);
			return val * .01f;
		};
	}

	StrToValFunc xen()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Xen));
			const auto val = p(text, 0.f);
			return val;
		};
	}

	StrToValFunc note()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.toLowerCase();
			auto val = p(text, -1.f);
			if (val >= 0.f && val < 128.f)
				return val;

			enum pitchclass { C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B, Num };
			enum class State { Pitchclass, FlatOrSharp, Parse, numStates };

			auto state = State::Pitchclass;

			for (auto i = 0; i < text.length(); ++i)
			{
				auto chr = text[i];

				if (state == State::Pitchclass)
				{
					if (chr == 'c')
						val = C;
					else if (chr == 'd')
						val = D;
					else if (chr == 'e')
						val = E;
					else if (chr == 'f')
						val = F;
					else if (chr == 'g')
						val = G;
					else if (chr == 'a')
						val = A;
					else if (chr == 'b')
						val = B;
					else
						return 69.f;

					state = State::FlatOrSharp;
				}
				else if (state == State::FlatOrSharp)
				{
					if (chr == '#')
						++val;
					else if (chr == 'b')
						--val;
					else
						--i;

					state = State::Parse;
				}
				else if (state == State::Parse)
				{
					auto newVal = p(text.substring(i), -1.f);
					if (newVal == -1.f)
						return 69.f;
					val += 12 + newVal * 12.f;
					while (val < 0.f)
						val += 12.f;
					return val;
				}
				else
					return 69.f;
			}

			return juce::jlimit(0.f, 127.f, val + 12.f);
		};
	}

	StrToValFunc pitch(const Xen& xenManager)
	{
		return[hzFunc = hz(), noteFunc = note(), &xen = xenManager](const String& txt)
		{
			auto freqHz = hzFunc(txt);
			if (freqHz != 0.f)
				return xen.freqHzToNote(freqHz);

			return noteFunc(txt);
		};
	}

	StrToValFunc q()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Q));
			const auto val = p(text, 40.f);
			return val;
		};
	}

	StrToValFunc slope()
	{
		return[p = parse()](const String& txt)
		{
			const auto text = txt.trimCharactersAtEnd(toString(Unit::Slope));
			const auto val = p(text, 0.f);
			return val / 12.f;
		};
	}

	StrToValFunc beats()
	{
		return[p = parse()](const String& txt)
		{
			enum Mode { Beats, Triplet, Dotted, NumModes };
			const auto lastChr = txt[txt.length() - 1];
			const auto mode = lastChr == 't' ? Mode::Triplet : lastChr == '.' ? Mode::Dotted : Mode::Beats;

			const auto text = mode == Mode::Beats ? txt : txt.substring(0, txt.length() - 1);
			auto val = p(text, 1.f / 16.f);
			if (mode == Mode::Triplet)
				val *= 1.666666666667f;
			else if (mode == Mode::Dotted)
				val *= 1.75f;
			return val;
		};
	}

	StrToValFunc legato()
	{
		return[p = parse()](const String& txt)
		{
			if (math::stringNegates(txt))
				return 0.f;
			return p(txt, 0.f);
		};
	}

	StrToValFunc filterType()
	{
		return[p = parse()](const String& txt)
		{
			auto text = txt.toLowerCase();
			if (text == "lp")
				return 0.f;
			else if (text == "hp")
				return 1.f;
			else if (text == "bp")
				return 2.f;
			else if (text == "br")
				return 3.f;
			else if (text == "ap")
				return 4.f;
			else if (text == "ls")
				return 5.f;
			else if (text == "hs")
				return 6.f;
			else if (text == "notch")
				return 7.f;
			else if (text == "bell")
				return 8.f;
			else
				return p(text, 0.f);
		};
	}

	StrToValFunc vowel()
	{
		return[p = parse()](const String& txt)
		{
			auto nTxt = txt.toLowerCase().removeCharacters(" ");

			for (auto i = 0; i < dsp::formant::NumVowelClasses; ++i)
			{
				const auto vowelClass = static_cast<dsp::formant::VowelClass>(i);
				const auto vowelStr = dsp::formant::toString(vowelClass).toLowerCase().removeCharacters(" ");
				if (nTxt == vowelStr)
					return static_cast<float>(i);
			}
			const auto val = p(txt, -1.f);
			if (val != -1.f)
				return 0.f;
			return val;
		};
	}
}

namespace param::valToStr
{
	ValToStrFunc mute()
	{
		return [](float v) { return v > .5f ? "Mute" : "Not Mute"; };
	}

	ValToStrFunc solo()
	{
		return [](float v) { return v > .5f ? "Solo" : "Not Solo"; };
	}

	ValToStrFunc power()
	{
		return [](float v) { return v > .5f ? "Enabled" : "Disabled"; };
	}

	ValToStrFunc percent()
	{
		return [](float v) { return String(std::round(v * 100.f)) + " " + toString(Unit::Percent); };
	}

	ValToStrFunc hz()
	{
		return [](float v)
			{
				if (v >= 10000.f)
					return String(v * .001).substring(0, 4) + " k" + toString(Unit::Hz);
				else if (v >= 1000.f)
					return String(v * .001).substring(0, 3) + " k" + toString(Unit::Hz);
				else
					return String(v).substring(0, 5) + " " + toString(Unit::Hz);
			};
	}

	ValToStrFunc phase()
	{
		return [](float v) { return String(std::round(v * 180.f)) + " " + toString(Unit::Degree); };
	}

	ValToStrFunc phase360()
	{
		return [](float v) { return String(std::round(v * 360.f)) + " " + toString(Unit::Degree); };
	}

	ValToStrFunc oct()
	{
		return [](float v) { return String(std::round(v)) + " " + toString(Unit::Octaves); };
	}

	ValToStrFunc octFloat()
	{
		return [](float v) { return String(v, 1) + " " + toString(Unit::Octaves); };
	}

	ValToStrFunc semi()
	{
		return [](float v) { return String(std::round(v)) + " " + toString(Unit::Semi); };
	}

	ValToStrFunc fine()
	{
		return [](float v) { return String(std::round(v * 100.f)) + " " + toString(Unit::Fine); };
	}

	ValToStrFunc ratio()
	{
		return [](float v)
			{
				const auto y = static_cast<int>(std::round(v * 100.f));
				return String(100 - y) + " : " + String(y);
			};
	}

	ValToStrFunc lrms()
	{
		return [](float v) { return v > .5f ? String("m/s") : String("l/r"); };
	}

	ValToStrFunc freeSync()
	{
		return [](float v) { return v > .5f ? String("sync") : String("free"); };
	}

	ValToStrFunc polarity()
	{
		return [](float v) { return v > .5f ? String("on") : String("off"); };
	}

	ValToStrFunc ms()
	{
		return [](float v)
			{
				if (v < 100.f)
				{
					v = std::round(v * 100.f) * .01f;
					return String(v) + " ms";
				}
				else if (v < 1000.f)
				{
					v = std::round(v * 10.f) * .1f;
					return String(v) + " ms";
				}
				else
				{
					v = std::round(v * .1f) * .01f;
					return String(v) + " sec";
				}
			};
	}

	ValToStrFunc db()
	{
		return [](float v) { return String(v, 1) + " " + toString(Unit::Decibel); };
	}

	ValToStrFunc empty()
	{
		return [](float) { return String(""); };
	}

	ValToStrFunc voices()
	{
		return [](float v)
			{
				return String(std::round(v)) + toString(Unit::Voices);
			};
	}

	ValToStrFunc pan(const Params& params)
	{
		return [&prms = params](float v)
			{
				if (v == 0.f)
					return String("C");

#if PPDHasStereoConfig
				const auto sc = prms[PID::StereoConfig];
				const auto vm = sc->getValMod();
				const auto isMidSide = vm > .5f;

				if (!isMidSide)
#endif
				{
					if (v == -1.f)
						return String("Left");
					else if (v == 1.f)
						return String("Right");
					else
						return String(std::round(v * 100.f)) + (v < 0.f ? " L" : " R");
				}
#if PPDHasStereoConfig
				else
				{
					if (v == -1.f)
						return String("Mid");
					else if (v == 1.f)
						return String("Side");
					else
						return String(std::round(v * 100.f)) + (v < 0.f ? " M" : " S");
				}
#endif
			};
	}

	ValToStrFunc xen(const Params& params)
	{
		return [&prms = params](float v)
			{
				const auto& snapParam = prms(PID::XenSnap);
				const auto snap = snapParam.getValMod() > .5f;
				if (snap)
					v = std::round(v);
				else
					v = std::round(v * 100.f) * .01f;
				return String(v) + " " + toString(Unit::Xen);
			};
	}

	ValToStrFunc note()
	{
		return [](float v)
			{
				if (v >= 0.f)
				{
					enum pitchclass { C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B, Num };

					const auto note = static_cast<int>(std::round(v));
					const auto octave = note / 12 - 1;
					const auto noteName = note % 12;
					return math::pitchclassToString(noteName) + String(octave);
				}
				return String("?");
			};
	}

	ValToStrFunc pitch(const Xen& xenManager)
	{
		return [noteFunc = note(), hzFunc = hz(), &xen = xenManager](float v)
			{
				return noteFunc(v) + "; " + hzFunc(xen.noteToFreqHz(v));
			};
	}

	ValToStrFunc q()
	{
		return [](float v)
			{
				v = std::round(v * 100.f) * .01f;
				return String(v) + " " + toString(Unit::Q);
			};
	}

	ValToStrFunc slope()
	{
		return [](float v)
			{
				v = std::round(v) * 12.f;
				return String(v) + " " + toString(Unit::Slope);
			};
	}

	ValToStrFunc beats()
	{
		enum Mode { Whole, Triplet, Dotted, NumModes };

		return [](float v)
			{
				if (v == 0.f)
					return String("0");

				const auto denormFloor = math::nextLowestPowTwoX(v);
				const auto denormFrac = v - denormFloor;
				const auto modeVal = denormFrac / denormFloor;
				const auto mode = modeVal < .66f ? Mode::Whole :
					modeVal < .75f ? Mode::Triplet :
					Mode::Dotted;
				String modeStr = mode == Mode::Whole ? String("") :
					mode == Mode::Triplet ? String("t") :
					String(".");

				auto denominator = 1.f / denormFloor;
				auto numerator = 1.f;
				if (denominator < 1.f)
				{
					numerator = denormFloor;
					denominator = 1.f;
				}

				return String(numerator) + " / " + String(denominator) + modeStr;
			};
	}

	ValToStrFunc legato()
	{
		return [](float v)
			{
				return v < .5f ? String("Off") : v < 1.5f ? String("On") : String("On+Sus");
			};
	}

	ValToStrFunc filterType()
	{
		return [](float v)
			{
				auto idx = static_cast<int>(std::round(v));
				switch (idx)
				{
				case 0: return String("LP");
				case 1: return String("HP");
				case 2: return String("BP");
				case 3: return String("BR");
				case 4: return String("AP");
				case 5: return String("LS");
				case 6: return String("HS");
				case 7: return String("Notch");
				case 8: return String("Bell");
				default: return String("");
				}
			};
	}

	ValToStrFunc vowel()
	{
		return [](float v)
			{
				v = std::round(v);
				const auto vInt = static_cast<int>(v);
				const auto vowelClass = static_cast<dsp::formant::VowelClass>(vInt);
				return dsp::formant::toString(vowelClass);
			};
	}
}

namespace param
{
	// pID, valDenormDefault, range, Unit
	extern Param* makeParam(PID id, float valDenormDefault = 1.f,
		const Range& range = { 0.f, 1.f }, Unit unit = Unit::Percent)
	{
		ValToStrFunc valToStrFunc;
		StrToValFunc strToValFunc;

		switch (unit)
		{
		case Unit::Power:
			valToStrFunc = valToStr::power();
			strToValFunc = strToVal::power();
			break;
		case Unit::Solo:
			valToStrFunc = valToStr::solo();
			strToValFunc = strToVal::solo();
			break;
		case Unit::Mute:
			valToStrFunc = valToStr::mute();
			strToValFunc = strToVal::mute();
			break;
		case Unit::Decibel:
			valToStrFunc = valToStr::db();
			strToValFunc = strToVal::db();
			break;
		case Unit::Ms:
			valToStrFunc = valToStr::ms();
			strToValFunc = strToVal::ms();
			break;
		case Unit::Percent:
			valToStrFunc = valToStr::percent();
			strToValFunc = strToVal::percent();
			break;
		case Unit::Hz:
			valToStrFunc = valToStr::hz();
			strToValFunc = strToVal::hz();
			break;
		case Unit::Ratio:
			valToStrFunc = valToStr::ratio();
			strToValFunc = strToVal::ratio();
			break;
		case Unit::Polarity:
			valToStrFunc = valToStr::polarity();
			strToValFunc = strToVal::polarity();
			break;
		case Unit::StereoConfig:
			valToStrFunc = valToStr::lrms();
			strToValFunc = strToVal::lrms();
			break;
		case Unit::Octaves:
			valToStrFunc = valToStr::oct();
			strToValFunc = strToVal::oct();
			break;
		case Unit::OctavesFloat:
			valToStrFunc = valToStr::octFloat();
			strToValFunc = strToVal::octFloat();
			break;
		case Unit::Semi:
			valToStrFunc = valToStr::semi();
			strToValFunc = strToVal::semi();
			break;
		case Unit::Fine:
			valToStrFunc = valToStr::fine();
			strToValFunc = strToVal::fine();
			break;
		case Unit::Voices:
			valToStrFunc = valToStr::voices();
			strToValFunc = strToVal::voices();
			break;
		case Unit::Note:
			valToStrFunc = valToStr::note();
			strToValFunc = strToVal::note();
			break;
		case Unit::Q:
			valToStrFunc = valToStr::q();
			strToValFunc = strToVal::q();
			break;
		case Unit::Slope:
			valToStrFunc = valToStr::slope();
			strToValFunc = strToVal::slope();
			break;
		case Unit::Beats:
			valToStrFunc = valToStr::beats();
			strToValFunc = strToVal::beats();
			break;
		case Unit::Legato:
			valToStrFunc = valToStr::legato();
			strToValFunc = strToVal::legato();
			break;
		case Unit::FilterType:
			valToStrFunc = valToStr::filterType();
			strToValFunc = strToVal::filterType();
			break;
		case Unit::Vowel:
			valToStrFunc = valToStr::vowel();
			strToValFunc = strToVal::vowel();
			break;
		default:
			valToStrFunc = [](float v) { return String(v); };
			strToValFunc = [p = strToVal::parse()](const String& s)
			{
				return p(s, 0.f);
			};
			break;
		}

		return new Param(id, range, valDenormDefault, valToStrFunc, strToValFunc, unit);
	}

	// pID, params
	extern Param* makeParamPan(PID id, const Params& params)
	{
		ValToStrFunc valToStrFunc = valToStr::pan(params);
		StrToValFunc strToValFunc = strToVal::pan(params);

		return new Param(id, { -1.f, 1.f }, 0.f, valToStrFunc, strToValFunc, Unit::Pan);
	}

	// pID, valDenormDefault, range, Xen
	extern Param* makeParamPitch(PID id, float valDenormDefault,
		const Range& range, const Xen& xen)
	{
		ValToStrFunc valToStrFunc = valToStr::pitch(xen);
		StrToValFunc strToValFunc = strToVal::pitch(xen);

		return new Param(id, range, valDenormDefault, valToStrFunc, strToValFunc, Unit::Pitch);
	}

	extern Param* makeParamXen(const Params& params)
	{
		constexpr auto MaxXen = static_cast<float>(PPDMaxXen);
		constexpr auto DefaultXen = 12.f;
		const auto range = makeRange::withCentre(3.f, MaxXen, DefaultXen);
		ValToStrFunc valToStrFunc = valToStr::xen(params);
		StrToValFunc strToValFunc = strToVal::xen();
		return new Param(PID::Xen, range, DefaultXen, valToStrFunc, strToValFunc, Unit::Xen);
	}

	extern Param* makeParam(PID id, float valDenormDefault, const Range& range,
		const ValToStrFunc& valToStrFunc, const StrToValFunc& strToValFunc = strToVal::standard(0.f))
	{
		return new Param(id, range, valDenormDefault, valToStrFunc, strToValFunc, Unit::Custom);
	}

	// PARAMS

	Params::Params(AudioProcessor& audioProcessor
#if PPDHasTuningEditor
		, const Xen&
#endif
	) :
		params(),
		modDepthAbsolute(false)
	{
		{ // HIGH LEVEL PARAMS:
			params.push_back(makeParam(PID::Macro, 1.f));

#if PPDIsNonlinear
			const auto gainInRange = makeRange::withCentre(PPDGainInMin, PPDGainInMax, 0.f);
			params.push_back(makeParam(PID::GainIn, 0.f, gainInRange, Unit::Decibel));
			params.push_back(makeParam(PID::UnityGain, 1.f, makeRange::toggle(), Unit::Polarity));
#endif
#if PPDIO == PPDIODryWet
			const auto gainRangeCentre = -20.f;
			const auto gainDryRange = makeRange::withCentre(PPDGainDryMin, PPDGainDryMax, gainRangeCentre);
			const auto gainWetRange = makeRange::withCentre(PPDGainWetMin, PPDGainWetMax, gainRangeCentre);
			params.push_back(makeParam(PID::GainDry, PPDGainDryMin, gainDryRange, Unit::Decibel));
			params.push_back(makeParam(PID::GainWet, -18.f, gainWetRange, Unit::Decibel));
#elif PPDIO == PPDIOWetMix
			const auto gainWetRange = makeRange::lin(PPDGainWetMin, PPDGainWetMax);
			params.push_back(makeParam(PID::GainWet, 0.f, gainWetRange, Unit::Decibel));
			params.push_back(makeParam(PID::Mix, 1.f));
#if PPDHasDelta
			params.push_back(makeParam(PID::Delta, 0.f, makeRange::toggle(), Unit::Power));
#endif
#endif
			const auto gainOutRange = makeRange::lin(PPDGainOutMin, PPDGainOutMax);
			params.push_back(makeParam(PID::GainOut, 0.f, gainOutRange, Unit::Decibel));
#if PPDHasStereoConfig
			params.push_back(makeParam(PID::StereoConfig, 0.f, makeRange::toggle(), Unit::StereoConfig));
#endif
#if PPDHasHQ
			params.push_back(makeParam(PID::HQ, 1.f, makeRange::toggle(), Unit::Power));
#endif
#if PPDHasLookahead
			params.push_back(makeParam(PID::Lookahead, 0.f, makeRange::toggle(), Unit::Power));
#endif
#if PPDHasTuningEditor
			// TUNING PARAMS:
			params.push_back(makeParamXen(*this));
			params.push_back(makeParam(PID::XenSnap, 1.f, makeRange::toggle(), Unit::Power));
			params.push_back(makeParam(PID::MasterTune, 440.f, makeRange::withCentre(420.f, 460.f, 440.f), Unit::Hz));
			params.push_back(makeParam(PID::AnchorPitch, 69.f, makeRange::stepped(0.f, 127.f), Unit::Note));
			params.push_back(makeParam(PID::PitchbendRange, 2.f, makeRange::stepped(1.f, 48.f), Unit::Semi));
#endif
			params.push_back(makeParam(PID::SoftClip, 0.f, makeRange::toggle(), Unit::Power));
			params.push_back(makeParam(PID::Power, 1.f, makeRange::toggle(), Unit::Power));
		}

		const auto SpreizungMin = dsp::modal::SpreizungMin;
		const auto SpreizungMax = dsp::modal::SpreizungMax;

		// LOW LEVEL PARAMS:
		params.push_back(makeParam(PID::NoiseBlend, 0.f));
		params.push_back(makeParam(PID::KeySelectorEnabled, 1.f, makeRange::toggle(), Unit::Power));
		//
		params.push_back(makeParam(PID::EnvGenAmpAttack, 4.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		params.push_back(makeParam(PID::EnvGenAmpDecay, 420.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		params.push_back(makeParam(PID::EnvGenAmpSustain, .8f, makeRange::lin(0.f, .999f)));
		params.push_back(makeParam(PID::EnvGenAmpRelease, 42.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		//
		params.push_back(makeParam(PID::ModSelect, 0.f, makeRange::stepped(0.f, 2.f),
			[](float v)
			{
				auto x = static_cast<int>(std::round(v));
				switch (x)
				{
				case 0: return "Env Gen";
				case 1: return "Env Fol";
				case 2: return "Random";
				default: return "Invalid, Biatch!";
				}
			}));
		//
		params.push_back(makeParam(PID::EnvGenModAttack, 4.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		params.push_back(makeParam(PID::EnvGenModDecay, 120.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		params.push_back(makeParam(PID::EnvGenModSustain, 0.f, makeRange::lin(0.f, .999f)));
		params.push_back(makeParam(PID::EnvGenModRelease, 42.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		params.push_back(makeParam(PID::EnvGenModAttackTS, 0.f, makeRange::beats(64.f, .5f, true), Unit::Beats));
		params.push_back(makeParam(PID::EnvGenModDecayTS, 1.f / 16.f, makeRange::beats(64.f, .5f, true), Unit::Beats));
		params.push_back(makeParam(PID::EnvGenModReleaseTS, 1.f / 16.f, makeRange::beats(64.f, .5f, true), Unit::Beats));
		params.push_back(makeParam(PID::EnvGenModTemposync, 1.f, makeRange::toggle(), Unit::Power));
		//
		params.push_back(makeParam(PID::EnvFolModGain, 0.f, makeRange::lin(-48.f, 48.f), Unit::Decibel));
		params.push_back(makeParam(PID::EnvFolModAttack, 4.f, makeRange::quad(0.f, 1000.f, 2), Unit::Ms));
		params.push_back(makeParam(PID::EnvFolModDecay, 120.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		params.push_back(makeParam(PID::EnvFolModSmooth, 42.f, makeRange::quad(0.f, 1000.f, 2), Unit::Ms));
		//
		params.push_back(makeParam(PID::RandModRateSync, 1.f / 16.f, makeRange::beats(64.f, .5f, false), Unit::Beats));
		params.push_back(makeParam(PID::RandModSmooth, 0.f));
		params.push_back(makeParam(PID::RandModComplex, 1.f, makeRange::lin(1.f, 8.f), Unit::OctavesFloat));
		params.push_back(makeParam(PID::RandModDropout, 0.f));
		//
		params.push_back(makeParam(PID::Polyphony, 15.f, makeRange::stepped(1.f, 15.f), Unit::Voices));
		//
		params.push_back(makeParam(PID::ModalOct, 0.f, makeRange::stepped(-4.f, 4.f), Unit::Octaves));
		params.push_back(makeParam(PID::ModalSemi, 0.f, makeRange::stepped(-12.f, 12.f), Unit::Semi));
		params.push_back(makeParam(PID::ModalBlend, 0.f));
		params.push_back(makeParam(PID::ModalBlendEnv, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::ModalBlendBreite, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::ModalSpreizung, 0.f, makeRange::lin(SpreizungMin, SpreizungMax)));
		params.push_back(makeParam(PID::ModalSpreizungEnv, 0.f, makeRange::lin(SpreizungMin * 2., SpreizungMax * 2.)));
		params.push_back(makeParam(PID::ModalSpreizungBreite, 0.f, makeRange::lin(SpreizungMin * 2., SpreizungMax * 2.)));
		params.push_back(makeParam(PID::ModalHarmonie, 0.f));
		params.push_back(makeParam(PID::ModalHarmonieEnv, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::ModalHarmonieBreite, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::ModalKraft, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::ModalKraftEnv, 0.f, makeRange::lin(-2.f, 2.f)));
		params.push_back(makeParam(PID::ModalKraftBreite, 0.f, makeRange::lin(-2.f, 2.f)));
		params.push_back(makeParam(PID::ModalResonanz, 1.f));
		params.push_back(makeParam(PID::ModalResonanzEnv, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::ModalResonanzBreite, 0.f, makeRange::lin(-1.f, 1.f)));
		//
		const auto maxVowelF = static_cast<float>(dsp::formant::NumVowelClasses - 1);
		const auto vowelADefault = static_cast<float>(dsp::formant::VowelClass::TenorA);
		const auto vowelBDefault = static_cast<float>(dsp::formant::VowelClass::TenorO);
		params.push_back(makeParam(PID::FormantA, vowelADefault, makeRange::stepped(0.f, maxVowelF), Unit::Vowel));
		params.push_back(makeParam(PID::FormantB, vowelBDefault, makeRange::stepped(0.f, maxVowelF), Unit::Vowel));
		params.push_back(makeParam(PID::FormantPos, 0.f));
		params.push_back(makeParam(PID::FormantPosEnv, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::FormantPosWidth, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::FormantQ, .85f, makeRange::lin(0.f, 1.f), Unit::Q));
		params.push_back(makeParam(PID::FormantQEnv, 0.f, makeRange::lin(-1.f, 1.f), Unit::Q));
		params.push_back(makeParam(PID::FormantQWidth, 0.f, makeRange::lin(-1.f, 1.f), Unit::Q));
		params.push_back(makeParam(PID::FormantDecay, 1000.f, makeRange::quad(0.f, 8000.f, 2), Unit::Ms));
		const auto formantMinGain = -48.f;
		params.push_back(makeParam(PID::FormantGain, formantMinGain, makeRange::withCentre(formantMinGain, 24.f, -0.f), Unit::Decibel));
		//
		params.push_back(makeParam(PID::CombOct, 0.f, makeRange::stepped(-4.f, 4.f), Unit::Octaves));
		params.push_back(makeParam(PID::CombSemi, 0.f, makeRange::stepped(-12.f, 12.f), Unit::Semi));
		params.push_back(makeParam(PID::CombUnison, 0.f, makeRange::lin(0.f, 1.f)));
		params.push_back(makeParam(PID::CombFeedback, 0.f, makeRange::lin(-1.f, 1.f)));
		params.push_back(makeParam(PID::CombFeedbackEnv, 0.f, makeRange::lin(-2.f, 2.f)));
		params.push_back(makeParam(PID::CombFeedbackWidth, 0.f, makeRange::lin(-1.f, 1.f)));
		//
		const auto numOctavesDamp = 8.f;
		params.push_back(makeParam(PID::Damp, numOctavesDamp, makeRange::lin(0.f, numOctavesDamp), Unit::OctavesFloat));
		params.push_back(makeParam(PID::DampEnv, 0.f, makeRange::lin(-numOctavesDamp, numOctavesDamp), Unit::OctavesFloat));
		params.push_back(makeParam(PID::DampWidth, 0.f, makeRange::lin(-2., 2.), Unit::OctavesFloat));
		// LOW LEVEL PARAMS END

		for (auto param : params)
			audioProcessor.addParameter(param);
	}

	void Params::loadPatch(const State& state)
	{
		const String idStr("params/");
		const auto mda = state.get(idStr + "mdabs");
		if (mda != nullptr)
			setModDepthAbsolute(static_cast<int>(*mda) != 0);

		for (auto param : params)
			param->loadPatch(state);
	}

	void Params::savePatch(State& state) const
	{
		for (auto param : params)
			param->savePatch(state);

		const String idStr("params/");
		state.set(idStr + "mdabs", (isModDepthAbsolute() ? 1 : 0));
	}

	int Params::getParamIdx(const String& nameOrID) const
	{
		for (auto p = 0; p < params.size(); ++p)
		{
			const auto pName = toString(params[p]->id);
			if (nameOrID == pName || nameOrID == toID(pName))
				return p;
		}
		return -1;
	}

	size_t Params::numParams() const noexcept
	{
		return params.size();
	}

	void Params::modulate(float modSrc) noexcept
	{
		for (auto i = 1; i < NumParams; ++i)
		{
			params[i]->startModulation();
			params[i]->modulate(modSrc);
			params[i]->endModulation();
		}
	}

	Param* Params::operator[](int i) noexcept { return params[i]; }
	const Param* Params::operator[](int i) const noexcept { return params[i]; }
	Param* Params::operator[](PID p) noexcept { return params[static_cast<int>(p)]; }
	const Param* Params::operator[](PID p) const noexcept { return params[static_cast<int>(p)]; }

	Param& Params::operator()(int i) noexcept { return *params[i]; }
	const Param& Params::operator()(int i) const noexcept { return *params[i]; }
	Param& Params::operator()(PID p) noexcept { return *params[static_cast<int>(p)]; }
	const Param& Params::operator()(PID p) const noexcept { return *params[static_cast<int>(p)]; }

	Params::Parameters& Params::data() noexcept
	{
		return params;
	}

	const Params::Parameters& Params::data() const noexcept
	{
		return params;
	}

	bool Params::isModDepthAbsolute() const noexcept
	{
		return modDepthAbsolute.load();
	}

	void Params::setModDepthAbsolute(bool e) noexcept
	{
		modDepthAbsolute.store(e);
		for (auto& p : params)
			p->setModDepthAbsolute(e);
	}

	void Params::switchModDepthAbsolute() noexcept
	{
		setModDepthAbsolute(!isModDepthAbsolute());
	}
}