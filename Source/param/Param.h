#pragma once
#include <juce_core/juce_core.h>
#include "juce_audio_processors/juce_audio_processors.h"
#include "../arch/XenManager.h"
#include "../arch/State.h"

namespace param
{
	using String = juce::String;
	
	String toID(const String&);

	enum class PID
	{
		// high level parameters:
		Macro,
#if PPDIsNonlinear
		GainIn,
		UnityGain,
#endif
#if PPDIO == PPDIODryWet
		GainDry,
		GainWet,
#elif PPDIO == PPDIOWetMix
		GainWet,
		Mix,
		Delta,
#endif
		GainOut,
#if PPDHasStereoConfig
		StereoConfig,
#endif
#if PPDHasHQ
		HQ,
#endif
#if PPDHasLookahead
		Lookahead,
#endif
		// tuning parameters:
#if PPDHasTuningEditor
		Xen,
		MasterTune,
		AnchorPitch,
		PitchbendRange,
#endif
		Power,

		// inputs:
		NoiseBlend,
		KeySelectorEnabled,
		// modsys:
		EnvGenAmpAttack,
		EnvGenAmpDecay,
		EnvGenAmpSustain,
		EnvGenAmpRelease,
		EnvGenModAttack,
		EnvGenModDecay,
		EnvGenModSustain,
		EnvGenModRelease,
		// modal
		ModalOct,
		ModalSemi,
		ModalSmartKeytrack,
		ModalSmartKeytrackEnv,
		ModalSmartKeytrackBreite,
		ModalBlend,
		ModalBlendEnv,
		ModalBlendBreite,
		ModalSpreizung,
		ModalSpreizungEnv,
		ModalSpreizungBreite,
		ModalHarmonie,
		ModalHarmonieEnv,
		ModalHarmonieBreite,
		ModalKraft,
		ModalKraftEnv,
		ModalKraftBreite,
		ModalResonanz,
		ModalResonanzEnv,
		ModalResonanzBreite,
		ModalResoDamp,
		ModalResoDampEnv,
		ModalResoDampBreite,
		// comb:
		CombOct,
		CombSemi,
		CombFeedback,
		CombFeedbackEnv,
		CombFeedbackWidth,
		CombAPReso,
		CombAPResoEnv,
		CombAPResoWidth,
		CombAPShape,
		CombAPShapeEnv,
		CombAPShapeWidth,
		//
		NumParams
	};
	static constexpr int NumParams = static_cast<int>(PID::NumParams);
	static constexpr int MinLowLevelIdx = static_cast<int>(PID::Power) + 1;
	static constexpr int NumLowLevelParams = NumParams - MinLowLevelIdx;

	/* pID, offset */
	PID ll(PID, int) noexcept;

	/* pID, offset */
	PID offset(PID, int) noexcept;

	String toString(PID);

	PID toPID(const String&);

	/* pIDs, text, seperatorChr */
	void toPIDs(std::vector<PID>&, const String&, const String&);

	String toTooltip(PID);

	enum class Unit
	{
		Power,
		Solo,
		Mute,
		Percent,
		Hz,
		Beats,
		Degree,
		Octaves,
		Semi,
		Fine,
		Ms,
		Decibel,
		Ratio,
		Polarity,
		StereoConfig,
		Voices,
		Pan,
		Xen,
		Note,
		Pitch,
		Q,
		Slope,
		Legato,
		Custom,
		FilterType,
		NumUnits
	};

	using CharPtr = juce::CharPointer_UTF8;

	String toString(Unit);

	using ValToStrFunc = std::function<String(float)>;
	using StrToValFunc = std::function<float(const String&)>;

	using Range = juce::NormalisableRange<float>;
	using ParameterBase = juce::AudioProcessorParameter;
	
	using Xen = arch::XenManager&;
	using State = arch::State;

	class Param :
		public ParameterBase
	{
		static constexpr float BiasEps = .000001f;
	public:

		struct Mod
		{
			Mod();

			std::atomic<float> depth, bias;
		};

		/* pID, range, valDenormDefault, valToStr, strToVal, unit */
		Param(const PID, const Range&, const float,
			const ValToStrFunc&, const StrToValFunc&,
			const Unit = Unit::NumUnits);

		void savePatch(State&) const;

		void loadPatch(State&);

		//called by host, normalized, thread-safe
		float getValue() const override;

		float getValueDenorm() const noexcept;

		// called by host, normalized, avoid locks, not used by editor
		void setValue(float) override;

		// setValue without the lock
		void setValueFromEditor(float) noexcept;

		// called by editor
		bool isInGesture() const noexcept;

		void setValueWithGesture(float/*norm*/);

		void beginGesture();

		void endGesture();

		float getModDepth() const noexcept;

		void setModDepth(float) noexcept;

		/* modSource */
		float calcValModOf(float) const noexcept;

		float getValMod() const noexcept;

		float getValModDenorm() const noexcept;

		void setModBias(float) noexcept;

		float getModBias() const noexcept;

		void setModulationDefault() noexcept;

		/* norm */
		void setDefaultValue(float) noexcept;

		void startModulation() noexcept;

		void modulate(float) noexcept;

		void endModulation() noexcept;

		float getDefaultValue() const override;

		String getName(int) const override;

		// units of param (hz, % etc.)
		String getLabel() const override;

		// string of norm val
		String getText(float /*norm*/, int) const override;

		// string to norm val
		float getValueForText(const String&) const override;

		// string to denorm val
		float getValForTextDenorm(const String&) const;

		String _toString();

		int getNumSteps() const override;

		bool isLocked() const noexcept;
		void setLocked(bool) noexcept;
		void switchLock() noexcept;

		void setModDepthAbsolute(bool) noexcept;

		/* start, end, bias[0,1], x */
		static float biased(float, float, float, float) noexcept;

		const PID id;
		const Range range;
	protected:
		float valDenormDefault, valInternal;
		Mod mod;
		std::atomic<float> valNorm, valMod;
		ValToStrFunc valToStr;
		StrToValFunc strToVal;
		Unit unit;

		std::atomic<bool> locked, inGesture;

		bool modDepthAbsolute;
	};

	struct Params
	{
		using AudioProcessor = juce::AudioProcessor;
		using Parameters = std::vector<Param*>;

		Params(AudioProcessor&
#if PPDHasTuningEditor
			, const Xen&
#endif
		);

		void loadPatch(State&);

		void savePatch(State&) const;

		int getParamIdx(const String& /*nameOrID*/) const;

		size_t numParams() const noexcept;

		void modulate(float modSrc) noexcept;

		bool isModDepthAbsolute() const noexcept;
		void setModDepthAbsolute(bool) noexcept;
		void switchModDepthAbsolute() noexcept;

		Param* operator[](int) noexcept;
		const Param* operator[](int) const noexcept;
		Param* operator[](PID) noexcept;
		const Param* operator[](PID) const noexcept;

		Param& operator()(int) noexcept;
		const Param& operator()(int) const noexcept;
		Param& operator()(PID) noexcept;
		const Param& operator()(PID) const noexcept;

		Parameters& data() noexcept;
		const Parameters& data() const noexcept;
	protected:
		Parameters params;
		std::atomic<float> modDepthAbsolute;
	};

	namespace strToVal
	{
		std::function<float(String, const float/*altVal*/)> parse();

		StrToValFunc power();
		StrToValFunc solo();
		StrToValFunc mute();
		StrToValFunc percent();
		StrToValFunc hz();
		StrToValFunc phase();
		StrToValFunc oct();
		StrToValFunc semi();
		StrToValFunc fine();
		StrToValFunc ratio();
		StrToValFunc lrms();
		StrToValFunc freeSync();
		StrToValFunc polarity();
		StrToValFunc ms();
		StrToValFunc db();
		StrToValFunc voices();
		StrToValFunc pan(const Params&);
		StrToValFunc note();
		StrToValFunc pitch(const Xen&);
		StrToValFunc q();
		StrToValFunc slope();
		StrToValFunc beats();
		StrToValFunc legato();
		StrToValFunc filterType();
	}

	namespace valToStr
	{
		ValToStrFunc mute();
		ValToStrFunc solo();
		ValToStrFunc power();
		ValToStrFunc percent();
		ValToStrFunc hz();
		ValToStrFunc phase();
		ValToStrFunc phase360();
		ValToStrFunc oct();
		ValToStrFunc semi();
		ValToStrFunc fine();
		ValToStrFunc ratio();
		ValToStrFunc lrms();
		ValToStrFunc freeSync();
		ValToStrFunc polarity();
		ValToStrFunc ms();
		ValToStrFunc db();
		ValToStrFunc empty();
		ValToStrFunc voices();
		ValToStrFunc pan(const Params&);
		ValToStrFunc note();
		ValToStrFunc pitch(const Xen&);
		ValToStrFunc q();
		ValToStrFunc slope();
		ValToStrFunc beats();
		ValToStrFunc legato();
		ValToStrFunc filterType();
	}
}