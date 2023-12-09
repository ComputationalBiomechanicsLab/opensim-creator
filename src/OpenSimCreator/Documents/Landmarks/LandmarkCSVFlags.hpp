#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc::lm
{
    enum class LandmarkCSVFlags {
        None     = 0,
        NoHeader = 1<<0,
        NoNames  = 1<<1,
    };

    constexpr LandmarkCSVFlags operator|(LandmarkCSVFlags lhs, LandmarkCSVFlags rhs)
    {
        return static_cast<LandmarkCSVFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }

    constexpr bool operator&(LandmarkCSVFlags lhs, LandmarkCSVFlags rhs)
    {
        return (osc::to_underlying(lhs) & osc::to_underlying(rhs)) != 0;
    }
}
