#pragma once

#include <libopynsim/Utils/LandmarkPair3D.h>
#include <liboscar/utils/StringName.h>

namespace osc
{
    struct NamedLandmarkPair3D final : public opyn::LandmarkPair3D<float> {
        StringName name;
    };
}
