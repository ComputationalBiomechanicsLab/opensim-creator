#include "VariantType.hpp"

#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Variant/VariantTypeList.hpp>
#include <oscar/Variant/VariantTypeTraits.hpp>

#include <array>
#include <string>

std::string osc::to_string(VariantType v)
{
    auto constexpr lut = []<VariantType... Types>(NonTypelist<VariantType, Types...>)
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
