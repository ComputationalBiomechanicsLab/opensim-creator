#pragma once

#include <liboscar/utilities/typelist.h>
#include <liboscar/maths/vector.h>

#include <variant>

namespace opyn
{
    using SupportedOutputValueTypes = osc::Typelist<
        double,
        osc::Vector3d
        // TODO:
        //  osc::Vector2d,
        //  osc::Vector3d,
        //  SpatialVector,
        //  bool,
        //  RigidTransform,
        //  Rotation,
        //  std::vector<float>
    >;

    using OutputValue = osc::VariantOfTypelistElements<SupportedOutputValueTypes>;
}
