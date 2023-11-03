#pragma once

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace osc
{
    enum class OverlayDecorationOptionFlags : uint32_t {
        None = 0,
        DrawXZGrid = 1<<0,
        DrawXYGrid = 1<<1,
        DrawYZGrid = 1<<2,
        DrawAxisLines = 1<<3,
        DrawAABBs = 1<<4,
        DrawBVH = 1<<5,
        NUM_OPTIONS = 6,

        Default = None,
    };

    constexpr bool operator&(OverlayDecorationOptionFlags a, OverlayDecorationOptionFlags b)
    {
        using Underlying = std::underlying_type_t<OverlayDecorationOptionFlags>;
        return (static_cast<Underlying>(a) & static_cast<Underlying>(b)) != 0;
    }

    constexpr void SetOption(OverlayDecorationOptionFlags& flags, OverlayDecorationOptionFlags flag, bool v)
    {
        using Underlying = std::underlying_type_t<OverlayDecorationOptionFlags>;

        if (v)
        {
            flags = static_cast<OverlayDecorationOptionFlags>(static_cast<Underlying>(flags) | static_cast<Underlying>(flag));
        }
        else
        {
            flags = static_cast<OverlayDecorationOptionFlags>(static_cast<Underlying>(flags) & ~static_cast<Underlying>(flag));
        }
    }

    constexpr OverlayDecorationOptionFlags IthOption(size_t i)
    {
        i = i < NumOptions<OverlayDecorationOptionFlags>() ? i : 0;
        return static_cast<OverlayDecorationOptionFlags>(1<<i);
    }

    enum class OverlayDecorationOptionGroup {
        Alignment = 0,
        Development,
        NUM_OPTIONS,
    };

    struct OverlayDecorationOptionFlagsMetadata final {
        CStringView id;
        CStringView label;
        OverlayDecorationOptionGroup group;
        OverlayDecorationOptionFlags value;
    };
    CStringView GetLabel(OverlayDecorationOptionGroup);
    std::span<OverlayDecorationOptionFlagsMetadata const> GetAllOverlayDecorationOptionFlagsMetadata();
}
