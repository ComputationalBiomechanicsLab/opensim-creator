#pragma once

#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/NonTypelist.hpp>
#include <oscar/Variant/VariantType.hpp>

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
