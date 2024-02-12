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

    constexpr CubemapFace FirstCubemapFace()
    {
        return CubemapFace::PositiveX;
    }

    constexpr CubemapFace LastCubemapFace()
    {
        return CubemapFace::NegativeZ;
    }

    constexpr CubemapFace Next(CubemapFace face)
    {
        return static_cast<CubemapFace>(cpp23::to_underlying(face) + 1);
    }

    constexpr size_t ToIndex(CubemapFace face)
    {
        static_assert(cpp23::to_underlying(FirstCubemapFace()) == 0);
        static_assert(cpp23::to_underlying(LastCubemapFace()) - cpp23::to_underlying(FirstCubemapFace()) == 5);
        return static_cast<size_t>(face);
    }
}
