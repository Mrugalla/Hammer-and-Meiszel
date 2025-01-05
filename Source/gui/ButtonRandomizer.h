#pragma once
#include "Button.h"

namespace gui
{
    struct ButtonRandomizer :
        public Button
    {
        using RandFunc = std::function<void(Random&)>;

        ButtonRandomizer(Utils&);

        void add(PID);

        void add(Param*);

        void add(std::vector<Param*>&&);

        void add(const std::vector<Param*>&);

        void add(const RandFunc&);

        // isAbsolute
        void operator()(bool);

        std::vector<Param*> randomizables;
        std::vector<RandFunc> randFuncs;

    protected:
        void mouseEnter(const Mouse&) override;

        String makeTooltip();
    };
}