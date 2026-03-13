#pragma once

#include <liboscar/utilities/typelist.h>

#include <variant>

namespace opyn
{
    using SupportedOutputValueTypes = osc::Typelist<
        double
        // TODO:
        //  osc::Vector2d,
        //  osc::Vector3d,
        //  SpatialVector,
        //  bool,
        //  RigidTransform,
        //  Rotation,
        //  std::vector<float>
    >;

    using OutputValue = std::variant<
        double
    >;
}
