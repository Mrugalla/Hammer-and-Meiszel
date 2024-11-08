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

		Label title;
		Button buttonA, buttonB, buttonSolo;
		ButtonDropDown buttonDropDown;
		std::array<ModalMaterialEditor, 2> materialEditors;
		ModalParamsEditor params;
		DropDownMenu dropDown;

	private:
		void initTitle();

		void initButtonAB();

		void initButtonSolo();

		void initDropDown();
	};
}