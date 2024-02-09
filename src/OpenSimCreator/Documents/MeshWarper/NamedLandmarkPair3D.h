#pragma once

#include <OpenSimCreator/Utils/LandmarkPair3D.h>

#include <oscar/Utils/StringName.h>

namespace osc
{
    struct NamedLandmarkPair3D final : public LandmarkPair3D {
        StringName name;
    };
}
