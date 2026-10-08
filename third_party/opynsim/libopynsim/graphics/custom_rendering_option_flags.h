#pragma once

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>

#include <cstdint>
#include <span>
#include <utility>

namespace opyn
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

    constexpr void set_option(CustomRenderingOptionFlags& flags, CustomRenderingOptionFlags flag, bool v)
    {
        if (v) {
            flags = static_cast<CustomRenderingOptionFlags>(std::to_underlying(flags) | std::to_underlying(flag));
        }
        else {
            flags = static_cast<CustomRenderingOptionFlags>(std::to_underlying(flags) & ~std::to_underlying(flag));
        }
    }

    constexpr CustomRenderingOptionFlags custom_rendering_ith_option(size_t i)
    {
        i = i < osc::num_flags<CustomRenderingOptionFlags>() ? i : 0;
        return static_cast<CustomRenderingOptionFlags>(1<<i);
    }

    struct CustomRenderingOptionFlagsMetadata final {
        osc::CStringView id;
        osc::CStringView label;
        CustomRenderingOptionFlags value;
    };
    std::span<const CustomRenderingOptionFlagsMetadata> get_all_custom_rendering_option_flags_metadata();
}
