#pragma once
#include <vector>
#include <functional>

namespace gui
{
    namespace evt
    {
        enum class Type
        {
            ColourSchemeChanged,
            TooltipUpdated,
            ClickedEmpty,
            NumTypes
        };

        /* type, stuff */
        using Evt = std::function<void(const Type, const void*)>;

        struct System
        {
            struct Member
            {
                Member(System&);

                Member(System&, const Evt&);

                Member(System&, Evt&&);

                Member(const Member&);

                ~Member();

                void operator()(const Type, const void* = nullptr) const;

                System& sys;
                Evt evt;
            };

            using Members = std::vector<Member*>;

            System();

            /* type, stuff */
            void notify(const Type, const void* = nullptr);

        protected:
            Members members;

            void add(Member*);

            void remove(const Member*);
        };

        using Member = System::Member;
        using Members = System::Members;
    }
}