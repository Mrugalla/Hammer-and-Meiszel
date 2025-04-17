#pragma once
#include "TextEditor.h"

namespace gui
{
	struct ParameterEditor :
		public TextEditor
	{
		ParameterEditor(Utils&);

		void setActive(bool) override;

	private:
		std::vector<param::PID> pIDs;
	};
}