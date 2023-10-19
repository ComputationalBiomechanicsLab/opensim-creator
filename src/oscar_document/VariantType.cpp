#include "VariantType.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

std::string osc::doc::to_string(VariantType v)
{
    static auto const s_Lut = osc::to_array<std::string_view>(
    {
        "Bool",
        "Color",
        "Float",
        "Int",
        "String",
        "Vec3",
    });

    auto const idx = std::underlying_type_t<VariantType>(v);
    if (0 <= idx && idx < s_Lut.size())
    {
        return std::string{s_Lut[idx]};
    }
    else
    {
        return "Unknown";
    }
}
