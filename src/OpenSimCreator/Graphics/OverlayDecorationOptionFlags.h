#pragma once

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace osc
{
    enum class OverlayDecorationOptionFlags : uint32_t {
        None          = 0,
        drawXZGrid    = 1<<0,
        drawXYGrid    = 1<<1,
        drawYZGrid    = 1<<2,
        DrawAxisLines = 1<<3,
        drawAABBs     = 1<<4,
        drawBVH       = 1<<5,
        NUM_FLAGS        = 6,

        Default = None,
    };

    constexpr bool operator&(OverlayDecorationOptionFlags lhs, OverlayDecorationOptionFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    constexpr void SetOption(OverlayDecorationOptionFlags& flags, OverlayDecorationOptionFlags flag, bool v)
    {
        if (v)
        {
            flags = static_cast<OverlayDecorationOptionFlags>(cpp23::to_underlying(flags) | cpp23::to_underlying(flag));
        }
        else
        {
            flags = static_cast<OverlayDecorationOptionFlags>(cpp23::to_underlying(flags) & ~cpp23::to_underlying(flag));
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
