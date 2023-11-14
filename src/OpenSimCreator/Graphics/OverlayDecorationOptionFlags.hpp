#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>
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
        return (osc::to_underlying(a) & osc::to_underlying(b)) != 0;
    }

    constexpr void SetOption(OverlayDecorationOptionFlags& flags, OverlayDecorationOptionFlags flag, bool v)
    {
        if (v)
        {
            flags = static_cast<OverlayDecorationOptionFlags>(osc::to_underlying(flags) | osc::to_underlying(flag));
        }
        else
        {
            flags = static_cast<OverlayDecorationOptionFlags>(osc::to_underlying(flags) & ~osc::to_underlying(flag));
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
    CStringView getLabel(OverlayDecorationOptionGroup);
    std::span<OverlayDecorationOptionFlagsMetadata const> GetAllOverlayDecorationOptionFlagsMetadata();
}
