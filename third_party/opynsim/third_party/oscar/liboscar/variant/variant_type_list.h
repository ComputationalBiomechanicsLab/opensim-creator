#pragma once

#include <liboscar/utils/enum_helpers.h>
#include <liboscar/variant/variant_type.h>

namespace osc
{
    using VariantTypeList = OptionList<VariantType,
        VariantType::None,
        VariantType::Bool,
        VariantType::Color,
        VariantType::Float,
        VariantType::Int,
        VariantType::String,
        VariantType::StringName,
        VariantType::Vector2,
        VariantType::Vector3,
        VariantType::VariantArray
    >;
}
