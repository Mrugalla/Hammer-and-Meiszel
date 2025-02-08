#pragma once
#include "ModalFilterMaterialEditor.h"
#include "ModalParamsEditor.h"
#include "../DropDownMenu.h"
#include "../ButtonRandomizer.h"
#include <functional>

namespace gui
{
	struct ModalModuleEditor :
		public Comp
	{
		ModalModuleEditor(Utils&);

		void paint(Graphics&) override;

		void resized() override;

		Button buttonAB, buttonRatioFreqs, buttonSolo;
		ButtonDropDown buttonDropDownGens, buttonDropDownMisc;
		ButtonRandomizer buttonRandomizer;
		std::array<ModalMaterialEditor, 2> materialEditors;
		ModalParamsEditor params;
		DropDownMenu dropDownGens, dropDownMisc;
		arch::RandSeed randSeedVertical, randSeedHorizontal, randSeedFixedPartials;

		std::function<void()> updateMaterialFunc;
		int wannaUpdate;
	private:
		void initButtonAB();

		void initButtonRatioFreqs();

		void initButtonSolo();

		void initDropDown();

		void initRandomizer();
	};
}