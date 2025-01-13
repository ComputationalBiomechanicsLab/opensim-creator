#pragma once

#include <libOpenSimCreator/Utils/LandmarkPair3D.h>

#include <liboscar/Utils/StringName.h>

namespace osc
{
    struct NamedLandmarkPair3D final : public LandmarkPair3D {
        StringName name;
    };
}
