#pragma once
#include "../../../Using.h"
#include "../../../../arch/State.h"
#include <juce_dsp/juce_dsp.h>
#include "Axiom.h"

namespace dsp
{
	namespace modal
	{
		struct Partial
		{
			double mag, fc;
		};

		struct MaterialData
		{
			using Array = std::array<Partial, NumPartials>;

			MaterialData();

			// if idx >= NumPartialsKeytracked, fc is fixed frequency
			Partial& operator[](int) noexcept;

			// if idx >= NumPartialsKeytracked, fc is fixed frequency
			const Partial& operator[](int) const noexcept;

			double getMag(int) const noexcept;

			// if idx >= NumPartialsKeytracked, fc is fixed frequency
			double getFc(int) const noexcept;

			void copy(const MaterialData&) noexcept;

			Array& data() noexcept;
		private:
			std::array<Partial, NumPartials> partials;
		};

		struct MaterialDataStereo
		{
			MaterialDataStereo();

			// ch
			MaterialData& operator[](int) noexcept;

			// ch
			const MaterialData& operator[](int) const noexcept;

			// ch,i
			Partial& operator()(int, int) noexcept;

			// ch,i
			const Partial& operator()(int, int) const noexcept;

			// ch,i
			double getMag(int, int) const noexcept;

			// ch,i
			double getFc(int, int) const noexcept;

			// other, numChannels
			void copy(const MaterialDataStereo&, int) noexcept;
		private:
			std::array<MaterialData, 2> data;
		};

		struct Material
		{
			static constexpr int FFTOrder = 15;
			static constexpr int FFTSize = 1 << FFTOrder;
			using MaterialBuffer = std::array<float, FFTSize>;

			Material();

			// state, matStr
			void savePatch(arch::State&, const String&) const;

			// state, matStr
			void loadPatch(const arch::State&, const String&);

			// data, size
			void load(const char*, int);

			void load();

			void updatePeakInfosFromGUI() noexcept;

			void reportEndGesture() noexcept;

			void reportUpdate() noexcept;

			// sampleRate, samples, numChannels, numSamples
			void fillBuffer(float, const float* const*, int, int);

			MaterialBuffer buffer;
			MaterialData peakInfos;
			std::atomic<StatusMat> status;
			String name;
			float sampleRate;
			std::atomic<bool> soloing;

			struct PeakIndexInfo
			{
				std::vector<int> indexes;
			};
		private:

			// data, size
			void fillBuffer(const char*, int);
		};

		void generateSine(Material&);
		void generateSaw(Material&);
		void generateSquare(Material&);
		void generateFibonacci(Material&);
		void generatePrime(Material&);

		using ActivesArray = std::array<bool, NumPartials>;

		struct DualMaterial
		{
			DualMaterial();

			const bool updated() const noexcept;

			void reportUpdate() noexcept;

			Material& getMaterial(int) noexcept;

			const Material& getMaterial(int) const noexcept;

			const MaterialData& getMaterialData(int) const noexcept;

			const bool isActive(int) const noexcept;

			ActivesArray& getActives() noexcept;

		protected:
			std::array<Material, 2> materials;
			ActivesArray actives;
		};
	}
}