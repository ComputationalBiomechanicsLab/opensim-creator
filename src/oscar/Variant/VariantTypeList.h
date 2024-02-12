#pragma once

#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/NonTypelist.h>
#include <oscar/Variant/VariantType.h>

namespace osc
{
    using VariantTypeList = NonTypelist<VariantType,
        VariantType::Nil,
        VariantType::Bool,
        VariantType::Color,
        VariantType::Float,
        VariantType::Int,
        VariantType::String,
        VariantType::StringName,
        VariantType::Vec3
    >;
    static_assert(NonTypelistSizeV<VariantTypeList> == NumOptions<VariantType>());
}
