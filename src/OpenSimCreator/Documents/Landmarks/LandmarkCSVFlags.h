#pragma once

#include <oscar/Shims/Cpp23/utility.h>

namespace osc::lm
{
    enum class LandmarkCSVFlags {
        None     = 0,
        NoHeader = 1<<0,
        NoNames  = 1<<1,
    };

    constexpr LandmarkCSVFlags operator|(LandmarkCSVFlags lhs, LandmarkCSVFlags rhs)
    {
        return static_cast<LandmarkCSVFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr bool operator&(LandmarkCSVFlags lhs, LandmarkCSVFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }
}
