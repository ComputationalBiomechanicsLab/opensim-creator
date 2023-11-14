#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

#include <cstddef>

namespace osc
{
    enum class CubemapFace {
        PositiveX = 0,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ,
        NUM_OPTIONS,
    };

    constexpr CubemapFace FirstCubemapFace() noexcept
    {
        return CubemapFace::PositiveX;
    }

    constexpr CubemapFace LastCubemapFace() noexcept
    {
        return CubemapFace::NegativeZ;
    }

    constexpr CubemapFace Next(CubemapFace face) noexcept
    {
        return static_cast<CubemapFace>(osc::to_underlying(face) + 1);
    }

    constexpr size_t ToIndex(CubemapFace face)
    {
        static_assert(osc::to_underlying(FirstCubemapFace()) == 0);
        static_assert(osc::to_underlying(LastCubemapFace()) - osc::to_underlying(FirstCubemapFace()) == 5);
        return static_cast<size_t>(face);
    }
}
