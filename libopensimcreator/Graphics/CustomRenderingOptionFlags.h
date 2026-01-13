#pragma once

#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/EnumHelpers.h>

#include <cstdint>
#include <span>
#include <utility>

namespace osc
{
    enum class CustomRenderingOptionFlags : uint32_t {
        None                         = 0,
        DrawFloor                    = 1<<0,
        MeshNormals                  = 1<<1,
        Shadows                      = 1<<2,
        DrawSelectionRims            = 1<<3,
        OrderIndependentTransparency = 1<<4,
        NUM_FLAGS                    =    5,

        Default = DrawFloor | Shadows | DrawSelectionRims,
    };

    constexpr bool operator&(CustomRenderingOptionFlags lhs, CustomRenderingOptionFlags rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }

    constexpr void SetOption(CustomRenderingOptionFlags& flags, CustomRenderingOptionFlags flag, bool v)
    {
        if (v)
        {
            flags = static_cast<CustomRenderingOptionFlags>(std::to_underlying(flags) | std::to_underlying(flag));
        }
        else
        {
            flags = static_cast<CustomRenderingOptionFlags>(std::to_underlying(flags) & ~std::to_underlying(flag));
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
    std::span<const CustomRenderingOptionFlagsMetadata> GetAllCustomRenderingOptionFlagsMetadata();
}
