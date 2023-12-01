#pragma once

#include <OpenSimCreator/Utils/LandmarkPair3D.hpp>

#include <oscar/Utils/StringName.hpp>

namespace osc
{
    struct NamedLandmarkPair3D final : public LandmarkPair3D {
        StringName name;
    };
}
