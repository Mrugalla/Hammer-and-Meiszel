#include "Colours.h"
#include "Layout.h"
#define DisregardState false

namespace gui
{
    Colours Colours::c{};

    void setColour(Colours::Array& cols, Colours::ID cID, Colour col, Props*
#if !DisregardState
        props
#endif
    )
    {
        cols[static_cast<int>(cID)] = col;
#if !DisregardState
        props->setValue("col" + toString(cID), col.toString());
        props->save();
        props->sendChangeMessage();
#endif
    }

    void loadColour(Colours::Array& cols, Colours::ID cID, Props*
#if !DisregardState
        props
#endif
    )
    {
        const auto idx = static_cast<int>(cID);
#if DisregardState
		cols[idx] = toDefault(cID);
#else
        const auto cIDStr = "col" + toString(cID);
        const auto str = props->getValue(cIDStr, toDefault(cID).toString());
        cols[idx] = Colour::fromString(str);
        props->setValue(cIDStr, str);
#endif
    }

    Colours::Colours() :
        cols(),
        props(nullptr)
    {
    }

    void Colours::init(Props* p)
    {
        props = p;

        for(auto i = 0; i < NumColours; ++i)
            loadColour(cols, static_cast<ID>(i), props);
#if !DisregardState
        props->save();
        props->sendChangeMessage();
#endif
    }

    void Colours::set(Colour col, ID cID)
    {
        setColour(cols, cID, col, props);
    }

    Colour Colours::operator()(ID i) const noexcept
    {
        return get(static_cast<int>(i));
    }

    Colour Colours::operator()(int i) const noexcept
    {
        return get(i);
    }

    Colour Colours::get(int i) const noexcept
    {
        return cols[i];
    }

    Colour getColour(CID cID) noexcept
    {
        return Colours::c(cID);
    }

    String toString(CID cID)
    {
        switch (cID)
        {
        case Colours::ID::Bg: return "Background";
        case Colours::ID::Txt: return "Text";
        case Colours::ID::Interact: return "Interact";
        case Colours::ID::Mod: return "Modulation";
        case Colours::ID::Hover: return "Hover";
        case Colours::ID::Darken: return "Darken";
        default: return "Invalid";
        }
    }

    Colour toDefault(CID cID) noexcept
    {
        switch (cID)
        {
        case Colours::ID::Bg: return Colour(0xff0c6066);
        case Colours::ID::Txt: return Colour(0xffe2ffd3);
        case Colours::ID::Interact: return Colour(0xffd5ff00);
        case Colours::ID::Mod: return Colour(0xff001014);
        case Colours::ID::Hover: return Colour(0x84ffffff);
        case Colours::ID::Darken: return Colour(0xad15133b);
        default: return Colour(0xff000000);
        }
    }

    void setCol(Graphics& g, CID cID)
	{
		g.setColour(getColour(cID));
	}

    void setCol(Graphics& g, CID cID, float alpha)
    {
        g.setColour(getColour(cID).withMultipliedAlpha(alpha));
    }
}

#undef DisregardState