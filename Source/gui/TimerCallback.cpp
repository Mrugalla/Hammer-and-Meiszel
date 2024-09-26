#include "TimerCallback.h"

namespace gui
{
	TimerCallbacks::CB::CB(std::function<void()> _func, int _id, kFPS _fps, bool _active):
		cb(_func),
		phase(0.f),
		id(_id),
		fps(_fps),
		active(_active)
	{
	}

	void TimerCallbacks::CB::start(float startPhase) noexcept
	{
		phase = startPhase;
		active = true;
	}

	void TimerCallbacks::CB::stop(float endPhase) noexcept
	{
		phase = endPhase;
		active = false;
	}

	TimerCallbacks::TimerCallbacks() :
		Timer(),
		callbacks(),
		idx(0)
	{
		startTimerHz(FPS);
	}

	void TimerCallbacks::add(CB* cb)
	{
		auto& cbs = callbacks[static_cast<int>(cb->fps)];
		cbs.push_back(cb);
	}

	void TimerCallbacks::remove(CB* cb)
	{
		for (auto i = 0; i < NumFPSs; ++i)
		{
			auto& cbs = callbacks[i];
			const auto it = std::find(cbs.begin(), cbs.end(), cb);
			if (it != cbs.end())
			{
				cbs.erase(it);
				return remove(cb);
			}
		}
	}

	void TimerCallbacks::timerCallback()
	{
		++idx;

		for (auto i = 0; i < NumFPSs; ++i)
		{
			const auto& cbs = callbacks[i];
			const auto fps = static_cast<kFPS>(i);
			const auto fpsOrder = 1 << static_cast<int>(fps); // 2^0 = 1, 2^1 = 2, 2^2 = 4, 2^3 = 8, 2^4 = 16

			if (idx % fpsOrder == 0)
				for (auto cb : cbs)
					if(cb->active)
						cb->cb();
		}

		idx &= 31;
	}

	float secsToInc(float secs, cbFPS fps) noexcept
	{
		switch (fps)
		{
		case cbFPS::k30:
			return 1.f / (30.f * secs);
		case cbFPS::k15:
			return 1.f / (15.f * secs);
		case cbFPS::k7_5:
			return 1.f / (7.5f * secs);
		case cbFPS::k3_75:
			return 1.f / (3.75f * secs);
		default:
			return 1.f / (60.f * secs);
		}
	}

	float msToInc(float ms, cbFPS fps) noexcept
	{
		return secsToInc(ms * .001f, fps);
	}
}