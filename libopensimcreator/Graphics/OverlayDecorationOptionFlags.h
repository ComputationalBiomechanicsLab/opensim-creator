#pragma once

#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/EnumHelpers.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace osc
{
    enum class OverlayDecorationOptionFlags : uint32_t {
        None          = 0,
        DrawXZGrid    = 1<<0,
        DrawXYGrid    = 1<<1,
        DrawYZGrid    = 1<<2,
        DrawAxisLines = 1<<3,
        DrawAABBs     = 1<<4,
        DrawBVH       = 1<<5,
        NUM_FLAGS        = 6,

        Default = None,
    };

    constexpr bool operator&(OverlayDecorationOptionFlags lhs, OverlayDecorationOptionFlags rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }

    constexpr void SetOption(OverlayDecorationOptionFlags& flags, OverlayDecorationOptionFlags flag, bool v)
    {
        if (v)
        {
            flags = static_cast<OverlayDecorationOptionFlags>(std::to_underlying(flags) | std::to_underlying(flag));
        }
        else
        {
            flags = static_cast<OverlayDecorationOptionFlags>(std::to_underlying(flags) & ~std::to_underlying(flag));
        }
    }

    constexpr OverlayDecorationOptionFlags IthOption(size_t i)
    {
        i = i < num_flags<OverlayDecorationOptionFlags>() ? i : 0;
        return static_cast<OverlayDecorationOptionFlags>(1<<i);
    }

    enum class OverlayDecorationOptionGroup {
        Alignment,
        Development,
        NUM_OPTIONS,
    };

    struct OverlayDecorationOptionFlagsMetadata final {
        CStringView id;
        CStringView label;
        OverlayDecorationOptionGroup group;
        OverlayDecorationOptionFlags value;
    };
    CStringView getLabel(OverlayDecorationOptionGroup);
    std::span<const OverlayDecorationOptionFlagsMetadata> GetAllOverlayDecorationOptionFlagsMetadata();
}
