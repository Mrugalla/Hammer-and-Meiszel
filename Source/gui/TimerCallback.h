#pragma once
#include "Using.h"

namespace gui
{
	struct TimerCallbacks :
		public Timer
	{
		enum class kFPS
		{
			k60,
			k30,
			k15,
			k7_5,
			k3_75,
			k_1_875,
			NumFPSs
		};
		static constexpr int NumFPSs = static_cast<int>(kFPS::NumFPSs);
		
		struct CB
		{
			// function, id, fps, active
			CB(std::function<void()>, int, kFPS, bool);

			void start(float) noexcept;

			void stop(float) noexcept;

			std::function<void()> cb;
			float phase;
			int id;
			kFPS fps;
			bool active;
		};

		TimerCallbacks();

		void add(CB*);

		void remove(CB*);

	protected:
		using Callbacks = std::array<std::vector<CB*>, NumFPSs>;
		Callbacks callbacks;
		int idx;

		void timerCallback() override;
	};

	using Callback = TimerCallbacks::CB;
	using cbFPS = TimerCallbacks::kFPS;
	using Callbacks = std::vector<Callback>;

	float secsToInc(float secs, cbFPS fps) noexcept;

	float msToInc(float ms, cbFPS fps) noexcept;
}