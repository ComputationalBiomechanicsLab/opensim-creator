#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <cstddef>

namespace osc
{
    enum class CubemapFace {
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ,
        NUM_OPTIONS,
    };

    constexpr CubemapFace firstCubemapFace()
    {
        return CubemapFace::PositiveX;
    }

    constexpr CubemapFace lastCubemapFace()
    {
        return CubemapFace::NegativeZ;
    }

    constexpr CubemapFace next(CubemapFace face)
    {
        return static_cast<CubemapFace>(cpp23::to_underlying(face) + 1);
    }
}
