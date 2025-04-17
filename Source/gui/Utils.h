#pragma once
#include "Colours.h"
#include "Events.h"
#include "TimerCallback.h"

namespace gui
{
	class Utils
	{
		static constexpr float DragSpeed = .5f;
	public:
		// pluginTop, processor
		Utils(Component&, Processor&);

		void add(Callback*);
		void remove(Callback*);

		std::vector<Param*>& getAllParams() noexcept;
		const std::vector<Param*>& getAllParams() const noexcept;
		Param& getParam(PID) noexcept;
		const Param& getParam(PID) const noexcept;

		ValueTree getState() const noexcept;

		float getDragSpeed() const noexcept;

		float fontHeight() const noexcept;

		Point getScreenPosition() const noexcept;

		void resized();

		ValueTree savePatch();
		void loadPatch(const ValueTree&);

		Props& getProps() noexcept;

		void giveDAWKeyboardFocus();

		evt::System eventSystem;
		TimerCallbacks callbacks;
		Component& pluginTop;
		Processor& audioProcessor;
		Params& params;
		float thicc;
	};
}