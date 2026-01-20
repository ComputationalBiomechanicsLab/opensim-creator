#pragma once

#include <libopynsim/utilities/landmark_pair_3d.h>
#include <liboscar/utils/string_name.h>

namespace osc
{
    struct NamedLandmarkPair3D final : public opyn::landmark_pair_3d<float> {
        StringName name;
    };
}
