#pragma once
#include "TextEditor.h"

namespace gui
{
	struct ParameterEditor :
		public TextEditor
	{
		ParameterEditor(Utils& u) :
			TextEditor(u),
			pIDs()
		{
			onEnter = [&]()
			{
				setActive(false);
				for (auto& pID : pIDs)
				{
					auto& param = u.getParam(pID);
					const auto valDenormTxt = txt;
					const auto valDenorm = param.getValForTextDenorm(valDenormTxt);
					const auto valNorm = param.range.convertTo0to1(valDenorm);
					param.setValueWithGesture(valNorm);
				}
			};

			addEvt([&](evt::Type t, const void* stuff)
			{
				if (t == evt::Type::ParameterEditorShowUp)
				{
					const auto pluginScreenBounds = u.pluginTop.getScreenBounds();
					const auto screenBoundsParent = getParentComponent()->getScreenBounds();
					const auto screenBounds = getScreenBounds();
					const auto knobScreenBounds = *static_cast<const Bounds*>(stuff);
					const auto x = knobScreenBounds.getX() - pluginScreenBounds.getX();
					const auto y = knobScreenBounds.getY() - pluginScreenBounds.getY();
					setTopLeftPosition(x, y);
					setActive(true);
				}
				else if (t == evt::Type::ParameterEditorAssignParam)
				{
					pIDs = *static_cast<const std::vector<param::PID>*>(stuff);
					const auto& param = u.getParam(pIDs[0]);
					setText(param.getCurrentValueAsText());
					repaint();
				}
				else if (t == evt::Type::ParameterEditorVanish)
				{
					setActive(false);
				}
			});
		}

		void setActive(bool e) override
		{
			if (e) setVisible(e);
			TextEditor::setActive(e);
			if(!e) setVisible(e);
		}

	private:
		std::vector<param::PID> pIDs;
	};
}