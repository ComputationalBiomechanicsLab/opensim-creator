#pragma once

#include <nonstd/span.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstdint>
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

    constexpr bool operator&(CustomRenderingOptionFlags a, CustomRenderingOptionFlags b)
    {
        using Underlying = std::underlying_type_t<CustomRenderingOptionFlags>;
        return (static_cast<Underlying>(a) & static_cast<Underlying>(b)) != 0;
    }

    constexpr void SetOption(CustomRenderingOptionFlags& flags, CustomRenderingOptionFlags flag, bool v)
    {
        using Underlying = std::underlying_type_t<CustomRenderingOptionFlags>;
        if (v)
        {
            flags = static_cast<CustomRenderingOptionFlags>(static_cast<Underlying>(flags) | static_cast<Underlying>(flag));
        }
        else
        {
            flags = static_cast<CustomRenderingOptionFlags>(static_cast<Underlying>(flags) & ~static_cast<Underlying>(flag));
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
    nonstd::span<CustomRenderingOptionFlagsMetadata const> GetAllCustomRenderingOptionFlagsMetadata();
}
