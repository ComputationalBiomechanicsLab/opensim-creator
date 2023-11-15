#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstdint>
#include <span>
#include <type_traits>

namespace osc
{
    enum class CustomRenderingOptionFlags : uint32_t {
        None              = 0,
        DrawFloor         = 1<<0,
        MeshNormals       = 1<<1,
        Shadows           = 1<<2,
        DrawSelectionRims = 1<<3,
        NUM_OPTIONS = 4,

        Default = DrawFloor | Shadows | DrawSelectionRims,
    };

    constexpr bool operator&(CustomRenderingOptionFlags lhs, CustomRenderingOptionFlags rhs)
    {
        return (osc::to_underlying(lhs) & osc::to_underlying(rhs)) != 0;
    }

    constexpr void SetOption(CustomRenderingOptionFlags& flags, CustomRenderingOptionFlags flag, bool v)
    {
        if (v)
        {
            flags = static_cast<CustomRenderingOptionFlags>(osc::to_underlying(flags) | osc::to_underlying(flag));
        }
        else
        {
            flags = static_cast<CustomRenderingOptionFlags>(osc::to_underlying(flags) & ~osc::to_underlying(flag));
        }
    }

    constexpr CustomRenderingOptionFlags CustomRenderingIthOption(size_t i)
    {
        i = i < NumOptions<CustomRenderingOptionFlags>() ? i : 0;
        return static_cast<CustomRenderingOptionFlags>(1<<i);
    }

    struct CustomRenderingOptionFlagsMetadata final {
        CStringView id;
        CStringView label;
        CustomRenderingOptionFlags value;
    };
    std::span<CustomRenderingOptionFlagsMetadata const> GetAllCustomRenderingOptionFlagsMetadata();
}
