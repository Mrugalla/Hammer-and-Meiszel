#pragma once
#include "Using.h"

namespace gui
{
    struct Colours
    {
        enum class ID
        {
            Bg,
            Darken,
            Txt,
            Interact,
            Inactive,
            Hover,
            Mod,
            NumCols
        };

        static constexpr int NumColours = static_cast<int>(ID::NumCols);

        using Array = std::array<Colour, static_cast<int>(ID::NumCols)>;

        Colours();

        void init(Props*);

        void set(Colour, ID);

        Colour operator()(ID) const noexcept;

        Colour operator()(int) const noexcept;

        Colour get(int) const noexcept;

        static Colours c;
    private:
        Array cols;
        Props* props;
    };

    using CID = Colours::ID;

    Colour getColour(CID) noexcept;

    void setCol(Graphics&, CID);
}