#pragma once
#include "Button.h"
#include <random>

namespace gui
{
    struct ButtonRandomizer :
        public Button
    {
        struct Randomizer
        {
            // utils, randomizerID
            Randomizer(Utils&, String&&);

            // seedUp
            void updateSeed(bool);

            float operator()();

        private:
            std::random_device rd;
            std::mt19937 mt;
            std::uniform_real_distribution<float> dist;
            int seed;
        };

        using RandFunc = std::function<void(Randomizer&)>;

        // utils, randomizerID
        ButtonRandomizer(Utils&, String&&);

        void add(PID);

        void add(Param*);

        void add(std::vector<Param*>&&);

        void add(const std::vector<Param*>&);

        void add(const RandFunc&);

        // seedUp, isAbsolute
        void operator()(bool, bool);

        std::vector<Param*> randomizables;
        std::vector<RandFunc> randFuncs;
        Randomizer randomizer;
    protected:
        void mouseEnter(const Mouse&) override;

        String makeTooltip();
    };
}