#pragma once
#include "ModalFilterMaterialEditor.h"
#include "ModalParamsEditor.h"
#include "../DropDownMenu.h"

namespace gui
{
	struct ModalModuleEditor :
		public Comp
	{
		ModalModuleEditor(Utils&);

		void paint(Graphics&) override;

		void resized() override;

		Button buttonAB, buttonSolo;
		ButtonDropDown buttonDropDownGens, buttonDropDownMisc;
		std::array<ModalMaterialEditor, 2> materialEditors;
		ModalParamsEditor params;
		DropDownMenu dropDownGens, dropDownMisc;

	private:
		void initButtonAB();

		void initButtonSolo();

		void initDropDown();
	};
}