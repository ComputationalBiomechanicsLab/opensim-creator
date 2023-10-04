#pragma once

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace osc
{
    enum class OpenSimDecorationOptionFlags : uint32_t {
        None                                                = 0,
        ShouldShowScapulo                                   = 1<<0,
        ShouldShowEffectiveLinesOfActionForOrigin           = 1<<1,
        ShouldShowEffectiveLinesOfActionForInsertion        = 1<<2,
        ShouldShowAnatomicalMuscleLinesOfActionForOrigin    = 1<<3,
        ShouldShowAnatomicalMuscleLinesOfActionForInsertion = 1<<4,
        ShouldShowCentersOfMass                             = 1<<5,
        ShouldShowPointToPointSprings                       = 1<<6,
        ShouldShowContactForces                             = 1<<7,
        NUM_OPTIONS = 8,

        Default = ShouldShowPointToPointSprings,
    };

    constexpr bool operator&(OpenSimDecorationOptionFlags a, OpenSimDecorationOptionFlags b) noexcept
    {
        using Underlying = std::underlying_type_t<OpenSimDecorationOptionFlags>;
        return (static_cast<Underlying>(a) & static_cast<Underlying>(b)) != 0;
    }

    struct OpenSimDecorationOptionMetadata final {
        CStringView id;
        CStringView label;
        std::optional<CStringView> maybeDescription;
    };
    OpenSimDecorationOptionMetadata const& GetIthOptionMetadata(size_t);
    OpenSimDecorationOptionFlags GetIthOption(size_t);
    void SetIthOption(OpenSimDecorationOptionFlags&, size_t, bool);
    void SetOption(OpenSimDecorationOptionFlags&, OpenSimDecorationOptionFlags, bool);
}
