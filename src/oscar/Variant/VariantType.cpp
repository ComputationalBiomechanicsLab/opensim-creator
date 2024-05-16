#include "VariantType.h"

#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Variant/VariantTypeList.h>
#include <oscar/Variant/VariantTypeTraits.h>

#include <array>
#include <cstddef>
#include <string>

std::string osc::to_string(VariantType variant_type)
{
    auto constexpr lut = []<VariantType... Types>(OptionList<VariantType, Types...>)
    {
        return std::to_array({ VariantTypeTraits<Types>::name... });
    }(VariantTypeList{});

    return std::string{lut.at(to_index(variant_type))};
}
