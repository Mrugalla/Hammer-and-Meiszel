#include "ParameterEditor.h"

namespace gui
{
	ParameterEditor::ParameterEditor(Utils& u) :
		TextEditor(u),
		pIDs()
	{
		onEnter = [&]()
			{
				setActive(false);
				for (const auto pID : pIDs)
				{
					auto& param = u.getParam(pID);
					const auto valDenormTxt = txt;
					const auto valDenorm = param.getValForTextDenorm(valDenormTxt);
					const auto valLegal = param.range.snapToLegalValue(valDenorm);
					const auto valNorm = param.range.convertTo0to1(valLegal);
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

	void ParameterEditor::setActive(bool e)
	{
		if (e) setVisible(e);
		TextEditor::setActive(e);
		if (!e) setVisible(e);
	}
}