#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc
{
    // runtime flags for a scene el type
    //
    // helps the UI figure out what it should/shouldn't show for a particular type
    // without having to resort to peppering visitors everywhere
    enum class SceneElFlags {
        None              = 0,
        CanChangeLabel    = 1<<0,
        CanChangePosition = 1<<1,
        CanChangeRotation = 1<<2,
        CanChangeScale    = 1<<3,
        CanDelete         = 1<<4,
        CanSelect         = 1<<5,
        HasPhysicalSize   = 1<<6,
    };

    constexpr bool operator&(SceneElFlags lhs, SceneElFlags rhs)
    {
        return (osc::to_underlying(lhs) & osc::to_underlying(rhs)) != 0;
    }

    constexpr SceneElFlags operator|(SceneElFlags lhs, SceneElFlags rhs)
    {
        return static_cast<SceneElFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }
}