#pragma once

#include <utility>

namespace osc
{
    // runtime flags for a mesh importer object
    enum class MiObjectFlags {
        None              = 0,
        CanChangeLabel    = 1<<0,
        CanChangePosition = 1<<1,
        CanChangeRotation = 1<<2,
        CanChangeScale    = 1<<3,
        CanDelete         = 1<<4,
        CanSelect         = 1<<5,
        HasPhysicalSize   = 1<<6,
    };

    constexpr bool operator&(MiObjectFlags lhs, MiObjectFlags rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }

    constexpr MiObjectFlags operator|(MiObjectFlags lhs, MiObjectFlags rhs)
    {
        return static_cast<MiObjectFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }
}
