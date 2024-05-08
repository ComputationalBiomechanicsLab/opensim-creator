#pragma once

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstdint>
#include <span>

namespace osc
{
    enum class CustomRenderingOptionFlags : uint32_t {
        None              = 0,
        DrawFloor         = 1<<0,
        MeshNormals       = 1<<1,
        Shadows           = 1<<2,
        DrawSelectionRims = 1<<3,
        NUM_FLAGS         =    4,

        Default = DrawFloor | Shadows | DrawSelectionRims,
    };

    constexpr bool operator&(CustomRenderingOptionFlags lhs, CustomRenderingOptionFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    constexpr void SetOption(CustomRenderingOptionFlags& flags, CustomRenderingOptionFlags flag, bool v)
    {
        if (v)
        {
            flags = static_cast<CustomRenderingOptionFlags>(cpp23::to_underlying(flags) | cpp23::to_underlying(flag));
        }
        else
        {
            flags = static_cast<CustomRenderingOptionFlags>(cpp23::to_underlying(flags) & ~cpp23::to_underlying(flag));
        }
    }

    constexpr CustomRenderingOptionFlags CustomRenderingIthOption(size_t i)
    {
        i = i < num_flags<CustomRenderingOptionFlags>() ? i : 0;
        return static_cast<CustomRenderingOptionFlags>(1<<i);
    }

    struct CustomRenderingOptionFlagsMetadata final {
        CStringView id;
        CStringView label;
        CustomRenderingOptionFlags value;
    };
    std::span<CustomRenderingOptionFlagsMetadata const> GetAllCustomRenderingOptionFlagsMetadata();
}
