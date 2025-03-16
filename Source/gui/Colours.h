#pragma once
#include "Using.h"

namespace gui
{
    struct Colours
    {
        enum class ID
        {
            Bg,
            Txt,
            Interact,
            Mod,
            Hover,
            Darken,
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

    String toString(CID);

    Colour getColour(CID) noexcept;

    Colour toDefault(CID cID) noexcept;

    void setCol(Graphics&, CID);

    // g, cID, alpha
    void setCol(Graphics&, CID, float);
}