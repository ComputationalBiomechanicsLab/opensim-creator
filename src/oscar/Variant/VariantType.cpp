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

    const size_t idx = to_index(v);
    if (idx < size(lut)) {
        return std::string{lut[idx]};
    }
    else {
        return "Unknown";
    }
}
