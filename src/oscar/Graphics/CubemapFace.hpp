#pragma once

#include <cstddef>
#include <type_traits>

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
        using Underlying = std::underlying_type_t<CubemapFace>;
        return static_cast<CubemapFace>(static_cast<Underlying>(face) + 1);
    }

    constexpr size_t ToIndex(CubemapFace face)
    {
        using Underlying = std::underlying_type_t<CubemapFace>;
        static_assert(static_cast<Underlying>(FirstCubemapFace()) == 0);
        static_assert(static_cast<Underlying>(LastCubemapFace()) - static_cast<Underlying>(FirstCubemapFace()) == 5);
        return static_cast<size_t>(face);
    }
}
