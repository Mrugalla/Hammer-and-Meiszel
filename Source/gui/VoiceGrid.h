#pragma once
#include "Comp.h"

namespace gui
{
	template<size_t NumVoices>
	struct VoiceGrid :
		public Comp
	{
		static constexpr float NumVoicesF = static_cast<float>(NumVoices);
		static constexpr float NumVoicesInv = 1.f / NumVoicesF;
		using Voices = std::array<bool, NumVoices>;
		using UpdateFunc = std::function<bool(Voices&)>;

		VoiceGrid(Utils&);

		void init(const UpdateFunc&);

		void paint(Graphics&) override;

	protected:
		std::array<bool, NumVoices> voices;
	public:
		int poly;
	};
}