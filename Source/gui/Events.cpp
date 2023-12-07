#include "Events.h"

namespace gui
{
    namespace evt
    {
        System::Member::Member(System& _sys) :
            sys(_sys),
            evt(nullptr)
        {
        }

        System::Member::Member(System& _sys, const Evt& _evt) :
            sys(_sys),
            evt(_evt)
        {
            sys.add(this);
        }

        System::Member::Member(System& _sys, Evt&& _evt) :
            sys(_sys),
            evt(_evt)
        {
            sys.add(this);
        }

        System::Member::Member(const Member& other) :
            sys(other.sys),
            evt(other.evt)
        {
            sys.add(this);
        }

        System::Member::~Member()
        {
            sys.remove(this);
        }

        void System::Member::operator()(const Type type, const void* stuff) const
        {
            sys.notify(type, stuff);
        }

        //SYSTEM

        System::System() :
            members()
        {}

        void System::notify(const Type type, const void* stuff)
        {
            for (const auto member : members)
                member->evt(type, stuff);
        }

        void System::add(Member* m)
        {
            members.push_back(m);
        }

        void System::remove(const Member* m)
        {
            for (auto i = 0; i < members.size(); ++i)
                if (m == members[i])
                {
                    members.erase(members.begin() + i);
                    return;
                }
        }
    }
}

