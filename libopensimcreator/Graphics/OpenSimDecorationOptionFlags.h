#pragma once

#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/Flags.h>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace osc
{
    enum class OpenSimDecorationOptionFlag : uint32_t {
        None                                                = 0,
        ShouldShowScapulo                                   = 1<<0,
        ShouldShowEffectiveLinesOfActionForOrigin           = 1<<1,
        ShouldShowEffectiveLinesOfActionForInsertion        = 1<<2,
        ShouldShowAnatomicalMuscleLinesOfActionForOrigin    = 1<<3,
        ShouldShowAnatomicalMuscleLinesOfActionForInsertion = 1<<4,
        ShouldShowCentersOfMass                             = 1<<5,
        ShouldShowPointToPointSprings                       = 1<<6,
        ShouldShowContactForces                             = 1<<7,
        ShouldShowForceLinearComponent                      = 1<<8,
        ShouldShowForceAngularComponent                     = 1<<9,
        ShouldShowPointForces                               = 1<<10,
        ShouldShowScholz2015ObstacleContactHints            = 1<<11,
        NUM_FLAGS                                           =    12,

        Default = ShouldShowPointToPointSprings | ShouldShowScholz2015ObstacleContactHints,
    };
    using OpenSimDecorationOptionFlags = Flags<OpenSimDecorationOptionFlag>;

    struct OpenSimDecorationOptionMetadata final {
        CStringView id;
        CStringView label;
        std::optional<CStringView> maybeDescription;
    };
    const OpenSimDecorationOptionMetadata& GetIthOptionMetadata(size_t);
    OpenSimDecorationOptionFlag GetIthOption(size_t);
    void SetIthOption(OpenSimDecorationOptionFlags&, size_t, bool);
}
