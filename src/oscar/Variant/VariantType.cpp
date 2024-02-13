#include "VariantType.h"

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Variant/VariantTypeList.h>
#include <oscar/Variant/VariantTypeTraits.h>

#include <array>
#include <string>

std::string osc::to_string(VariantType v)
{
    auto constexpr lut = []<VariantType... Types>(OptionList<VariantType, Types...>)
    {
        return std::to_array({ VariantTypeTraits<Types>::name... });
    }(VariantTypeList{});

    auto const idx = cpp23::to_underlying(v);
    if (0 <= idx && idx < std::ssize(lut)) {
        return std::string{lut[idx]};
    }
    else {
        return "Unknown";
    }
}
