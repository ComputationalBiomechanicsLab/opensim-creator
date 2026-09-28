#pragma once

#include <liboscar/utilities/typelist.h>
#include <liboscar/maths/vector.h>

#include <variant>

namespace opyn
{
    using SupportedOutputValueTypes = osc::Typelist<
        double,
        osc::Vector3d
    >;

    using OutputValue = osc::VariantOfTypelistElements<SupportedOutputValueTypes>;
}
