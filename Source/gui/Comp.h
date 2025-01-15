#pragma once
#include "Utils.h"
#include "Layout.h"

namespace gui
{
	struct Comp :
		public Component
	{
		static constexpr float LockAlpha = .5f;
		static constexpr float AniLengthMs = 200.f;

		// utils, tooltip
		Comp(Utils&, const String & = "");

		~Comp();

		void setLocked(bool);

		void deregisterCallbacks();

		void add(const Callback&);

		void registerCallbacks();

		// xL, yL
		void initLayout(const String&, const String&);

		void addEvt(const evt::Evt&);

		void notify(const evt::Type, const void* = nullptr);

		void mouseEnter(const Mouse&) override;

		void mouseUp(const Mouse&) override;

		void setTooltip(const String&);

		Utils& utils;
		Layout layout;
		String tooltip;
		std::vector<evt::Member> members;
		Callbacks callbacks;
	};
}