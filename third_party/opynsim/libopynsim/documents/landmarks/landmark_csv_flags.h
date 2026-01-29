#pragma once

#include <utility>

namespace opyn
{
    enum class LandmarkCSVFlags {
        None     = 0,
        NoHeader = 1<<0,
        NoNames  = 1<<1,
    };

    constexpr LandmarkCSVFlags operator|(LandmarkCSVFlags lhs, LandmarkCSVFlags rhs)
    {
        return static_cast<LandmarkCSVFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    constexpr bool operator&(LandmarkCSVFlags lhs, LandmarkCSVFlags rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }
}
