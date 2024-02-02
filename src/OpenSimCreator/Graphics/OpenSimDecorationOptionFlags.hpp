#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

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
        NUM_FLAGS                                           =    8,

        Default = ShouldShowPointToPointSprings,
    };

    constexpr bool operator&(OpenSimDecorationOptionFlags lhs, OpenSimDecorationOptionFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
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
