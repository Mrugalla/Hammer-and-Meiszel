#pragma once
#include <juce_core/juce_core.h>
#include "../../../../arch/Math.h"

namespace dsp
{
	namespace formant
	{
		using String = juce::String;

		static constexpr int NumFormants = 5; // not to be adjusted (but if u still try, use numbers <= 5)

		struct FilterProps
		{
			// f in fc, bw in fc, gain in db
			FilterProps(double _f = 0., double _bw = 0., double gainDb = 0.) :
				f(_f), bw(_bw), posBw(0.), posGain(0.),
				gain(math::dbToAmp(gainDb)), fc(0.), bwFc(0.),
				sampleRate(0.)
			{
			}

			void prepare(double _sampleRate) noexcept
			{
				sampleRate = _sampleRate;
			}

			void blend(const FilterProps& a, const FilterProps& b, double pos) noexcept
			{
				f = a.f + pos * (b.f - a.f);
				posBw = a.bw + pos * (b.bw - a.bw);
				posGain = a.gain + pos * (b.gain - a.gain);
				fc = math::freqHzToFc(f, sampleRate);
			}

			void applyQ(double q) noexcept
			{
				bw = posBw * q;
				gain = posGain / q;
				bwFc = math::freqHzToFc(bw, sampleRate);
			}
		private:
			double f, bw, posBw, posGain;
		public:
			double gain, fc, bwFc;
		private:
			double sampleRate;
		};

		enum class VowelClass
		{
			SopranoA, SopranoE, SopranoI, SopranoO, SopranoU,
			AltoA, AltoE, AltoI, AltoO, AltoU,
			CounterTenorA, CounterTenorE, CounterTenorI, CounterTenorO, CounterTenorU,
			TenorA, TenorE, TenorI, TenorO, TenorU,
			BassA, BassE, BassI, BassO, BassU,
			NumVowelClasses
		};
		static constexpr int NumVowelClasses = static_cast<int>(VowelClass::NumVowelClasses);

		inline String toString(VowelClass f)
		{
			switch (f)
			{
			case VowelClass::SopranoA: return "Soprano A";
			case VowelClass::SopranoE: return "Soprano E";
			case VowelClass::SopranoI: return "Soprano I";
			case VowelClass::SopranoO: return "Soprano O";
			case VowelClass::SopranoU: return "Soprano U";
			case VowelClass::AltoA: return "Alto A";
			case VowelClass::AltoE: return "Alto E";
			case VowelClass::AltoI: return "Alto I";
			case VowelClass::AltoO: return "Alto O";
			case VowelClass::AltoU: return "Alto U";
			case VowelClass::CounterTenorA: return "Counter Tenor A";
			case VowelClass::CounterTenorE: return "Counter Tenor E";
			case VowelClass::CounterTenorI: return "Counter Tenor I";
			case VowelClass::CounterTenorO: return "Counter Tenor O";
			case VowelClass::CounterTenorU: return "Counter Tenor U";
			case VowelClass::TenorA: return "Tenor A";
			case VowelClass::TenorE: return "Tenor E";
			case VowelClass::TenorI: return "Tenor I";
			case VowelClass::TenorO: return "Tenor O";
			case VowelClass::TenorU: return "Tenor U";
			case VowelClass::BassA: return "Bass A";
			case VowelClass::BassE: return "Bass E";
			case VowelClass::BassI: return "Bass I";
			case VowelClass::BassO: return "Bass O";
			case VowelClass::BassU: return "Bass U";
			default: return "Invalid Vowel";
			}
		}

		struct Vowel
		{
			Vowel(double f1, double f2, double f3, double f4, double f5,
				double g1, double g2, double g3, double g4, double g5,
				double bw1, double bw2, double bw3, double bw4, double bw5, VowelClass vc) :
				formants
				{
					FilterProps{f1, bw1, g1},
					FilterProps{f2, bw2, g2},
					FilterProps{f3, bw3, g3},
					FilterProps{f4, bw4, g4},
					FilterProps{f5, bw5, g5}
				},
				vowelClass(vc)
			{
				auto maxGain = 0.;
				for (const auto& formant : formants)
					maxGain = std::max(maxGain, formant.gain);
				const auto gainInv = 1. / maxGain;
				for (auto& formant : formants)
					formant.gain *= gainInv;
			}

			Vowel() :
				formants
			{
				FilterProps{0., 0., 0.},
				FilterProps{0., 0., 0.},
				FilterProps{0., 0., 0.},
				FilterProps{0., 0., 0.},
				FilterProps{0., 0., 0.}
			},
				vowelClass(VowelClass::NumVowelClasses)
			{ }

			void prepare(double sampleRate)
			{
				for (auto& formant : formants)
					formant.prepare(sampleRate);
			}

			void blend(const Vowel& a, const Vowel& b, double pos) noexcept
			{
				for (auto i = 0; i < NumFormants; ++i)
					formants[i].blend(a.formants[i], b.formants[i], pos);
			}

			void applyQ(double q) noexcept
			{
				for (auto i = 0; i < NumFormants; ++i)
					formants[i].applyQ(q);
			}

			const FilterProps& getFormant(int i) const noexcept
			{
				return formants[i];
			}

			const VowelClass getVowelClass() const noexcept
			{
				return vowelClass;
			}
		private:
			std::array<FilterProps, NumFormants> formants;
			VowelClass vowelClass;
		};

		inline Vowel toVowel(VowelClass f) noexcept
		{
			switch (f)
			{
			case VowelClass::SopranoA: return { 800, 1150, 2900, 3900, 4590, 0, -6, -32, -20, -50, 80, 90, 120, 130, 140, VowelClass::SopranoA };
			case VowelClass::SopranoE: return { 350, 2000, 2800, 3600, 4950, 0, -20, -15, -40, -56, 60, 100, 120, 150, 200, VowelClass::SopranoE };
			case VowelClass::SopranoI: return { 270, 2140, 2950, 3900, 4950, 0, -12, -26, -26, -44, 60, 90, 100, 120, 120, VowelClass::SopranoI };
			case VowelClass::SopranoO: return { 450, 800, 2830, 3800, 4950, 0, -11, -22, -22, -50, 70, 80, 100, 130, 135, VowelClass::SopranoO };
			case VowelClass::SopranoU: return { 325, 700, 2700, 3800, 4950, 0, -16, -35, -40, -60, 50, 60, 170, 180, 200, VowelClass::SopranoU };
			case VowelClass::AltoA: return { 800, 1150, 2850, 3500, 4950, 0, -4, -20, -36, -60, 80, 90, 120, 130, 140, VowelClass::AltoA };
			case VowelClass::AltoE: return { 400, 1600, 2700, 3300, 4950, 0, -24, -30, -35, -60, 60, 80, 120, 150, 200, VowelClass::AltoE };
			case VowelClass::AltoI: return { 350, 1700, 2700, 3700, 4950, 0, -20, -30, -36, -50, 50, 100, 120, 150, 200, VowelClass::AltoI };
			case VowelClass::AltoO: return { 450, 800, 2830, 3500, 4950, 0, -9, -16, -28, -55, 70, 80, 100, 130, 135, VowelClass::AltoO };
			case VowelClass::AltoU: return { 325, 700, 2530, 3500, 4950, 0, -12, -30, -40, -64, 50, 60, 170, 180, 200, VowelClass::AltoU };
			case VowelClass::CounterTenorA: return { 660, 1120, 2750, 3000, 3350, 0, -6, -23, -24, -38, 80, 90, 120, 130, 140, VowelClass::CounterTenorA };
			case VowelClass::CounterTenorE: return { 440, 1800, 2700, 3000, 3300, 0, -14, -18, -20, -20, 70, 80, 100, 120, 120, VowelClass::CounterTenorE };
			case VowelClass::CounterTenorI: return { 270, 1850, 2900, 3350, 3590, 0, -24, -24, -36, -36, 40, 90, 100, 120, 120, VowelClass::CounterTenorI };
			case VowelClass::CounterTenorO: return { 430, 820, 2700, 3000, 3300, 0, -10, -26, -22, -34, 40, 80, 100, 120, 120, VowelClass::CounterTenorO };
			case VowelClass::CounterTenorU: return { 370, 630, 2750, 3000, 3400, 0, -20, -23, -30, -34, 40, 60, 100, 120, 120, VowelClass::CounterTenorU };
			case VowelClass::TenorA: return { 650, 1080, 2650, 2900, 3250, 0, -6, -7, -8, -22, 80, 90, 120, 130, 140, VowelClass::TenorA };
			case VowelClass::TenorE: return { 400, 1700, 2600, 3200, 3580, 0, -14, -12, -14, -20, 70, 80, 100, 120, 120, VowelClass::TenorE };
			case VowelClass::TenorI: return { 290, 1870, 2800, 3250, 3540, 0, -15, -18, -20, -30, 40, 90, 100, 120, 120, VowelClass::TenorI };
			case VowelClass::TenorO: return { 400, 800, 2600, 2800, 3000, 0, -10, -12, -12, -26, 40, 80, 100, 120, 120, VowelClass::TenorO };
			case VowelClass::TenorU: return { 350, 600, 2700, 2900, 3250, 0, -20, -17, -14, -26, 40, 60, 100, 120, 120, VowelClass::TenorU };
			case VowelClass::BassA: return { 600, 1040, 2250, 2450, 2750, 0, -7, -9, -9, -20, 60, 70, 110, 120, 130, VowelClass::BassA };
			case VowelClass::BassE: return { 400, 1620, 2400, 2800, 3100, 0, -12, -9, -12, -18, 40, 80, 100, 120, 120, VowelClass::BassE };
			case VowelClass::BassI: return { 250, 1750, 2600, 3050, 3340, 0, -30, -16, -22, -28, 60, 90, 100, 120, 120, VowelClass::BassI };
			case VowelClass::BassO: return { 400, 750, 2400, 2600, 2900, 0, -11, -21, -20, -40, 40, 80, 100, 120, 120, VowelClass::BassO };
			case VowelClass::BassU: return { 350, 600, 2400, 2675, 2950, 0, -20, -32, -28, -36, 40, 80, 100, 120, 120, VowelClass::BassU };
			default: return {};
			}
		}
	}
}