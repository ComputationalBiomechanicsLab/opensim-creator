#pragma once

#include <libopynsim/utilities/landmark_pair_3d.h>
#include <liboscar/utilities/string_name.h>

namespace osc
{
    /// Represents a named landmark pair in 3D space.
    struct MwNamedLandmarkPair3D final : public opyn::LandmarkPair3D<float> {
        StringName name;
    };
}
