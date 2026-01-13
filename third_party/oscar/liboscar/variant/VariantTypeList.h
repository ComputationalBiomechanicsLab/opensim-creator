#pragma once

#include <liboscar/utils/EnumHelpers.h>
#include <liboscar/variant/VariantType.h>

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
