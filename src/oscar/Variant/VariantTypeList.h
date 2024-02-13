#pragma once

#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Variant/VariantType.h>

namespace osc
{
    using VariantTypeList = OptionList<VariantType,
        VariantType::Nil,
        VariantType::Bool,
        VariantType::Color,
        VariantType::Float,
        VariantType::Int,
        VariantType::String,
        VariantType::StringName,
        VariantType::Vec3
    >;
}
