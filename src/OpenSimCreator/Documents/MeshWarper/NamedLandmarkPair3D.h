#pragma once

#include <oscar/Utils/StringName.h>
#include <oscar_simbody/LandmarkPair3D.h>

namespace osc
{
    struct NamedLandmarkPair3D final : public LandmarkPair3D {
        StringName name;
    };
}
