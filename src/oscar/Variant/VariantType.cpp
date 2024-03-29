#include "VariantType.h"

#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Variant/VariantTypeList.h>
#include <oscar/Variant/VariantTypeTraits.h>

#include <array>
#include <cstddef>
#include <string>

std::string osc::to_string(VariantType v)
{
    auto constexpr lut = []<VariantType... Types>(OptionList<VariantType, Types...>)
    {
        return std::to_array({ VariantTypeTraits<Types>::name... });
    }(VariantTypeList{});

    size_t const idx = ToIndex(v);
    if (idx < std::size(lut)) {
        return std::string{lut[idx]};
    }
    else {
        return "Unknown";
    }
}
