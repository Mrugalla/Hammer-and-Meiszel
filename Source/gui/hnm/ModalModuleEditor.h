#pragma once
#include "ModalFilterMaterialEditor.h"
#include "ModalPartialsFixedEditor.h"
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

		Button buttonAB, buttonSolo, buttonFixed;
		ButtonDropDown buttonDropDownGens, buttonDropDownProcess;
		ButtonRandomizer buttonRandomizer;
		std::array<ModalMaterialEditor, 2> materialEditors;
		std::array<ModalPartialsFixedEditor, 2> partialsFixedEditors;
		ModalParamsEditor params;
		DropDownMenu dropDownGens, dropDownProcess;
		arch::RandSeed randSeedVertical, randSeedHorizontal, randSeedFixedPartials;
	private:
		void initButtonAB();

		void initButtonSolo();

		void initDropDown();

		void initRandomizer();
	};
}