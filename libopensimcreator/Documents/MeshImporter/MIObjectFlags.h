#pragma once

#include <utility>

namespace osc::mi
{
    // runtime flags for a mesh importer object
    enum class MIObjectFlags {
        None              = 0,
        CanChangeLabel    = 1<<0,
        CanChangePosition = 1<<1,
        CanChangeRotation = 1<<2,
        CanChangeScale    = 1<<3,
        CanDelete         = 1<<4,
        CanSelect         = 1<<5,
        HasPhysicalSize   = 1<<6,
    };

    constexpr bool operator&(MIObjectFlags lhs, MIObjectFlags rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }

    constexpr MIObjectFlags operator|(MIObjectFlags lhs, MIObjectFlags rhs)
    {
        return static_cast<MIObjectFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }
}
