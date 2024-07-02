#include "Colours.h"
#define DisregardState true

namespace gui
{
    Colours Colours::c{};

    String toString(Colours::ID cID)
    {
        switch (cID)
        {
        case Colours::ID::Bg: return "ColBg";
        case Colours::ID::Txt: return "ColTxt";
        case Colours::ID::Interact: return "ColInteract";
        case Colours::ID::Inactive: return "ColInactive";
        case Colours::ID::Hover: return "ColHover";
        case Colours::ID::Mod: return "ColMod";
        case Colours::ID::Darken: return "ColDarken";
        default: return "ColInvalid";
        }
    }

    Colour toDefault(Colours::ID cID) noexcept
    {
        switch (cID)
        {
        case Colours::ID::Bg: return Colour(0xff1d2022);
        case Colours::ID::Txt: return Colour(0xff851cfd);
        case Colours::ID::Interact: return Colour(0xffd134af);
        case Colours::ID::Inactive: return Colour(0xffc1c1c1);
        case Colours::ID::Hover: return Colour(0x77ffffff);
        case Colours::ID::Mod: return Colour(0xffb6153d);
        case Colours::ID::Darken: return Colour(0x88000000);
        default: return Colour(0xff000000);
        }
    }

    void setColour(Colours::Array& cols, Colours::ID cID, Colour col, Props*
#if !DisregardState
        props
#endif
    )
    {
        cols[static_cast<int>(cID)] = col;
#if !DisregardState
        props->setValue(toString(cID), col.toString());
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
        const auto cIDStr = toString(cID);
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

    void setCol(Graphics& g, CID cID)
	{
		g.setColour(getColour(cID));
	}
}

#undef DisregardState