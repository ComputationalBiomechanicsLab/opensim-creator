#include "VariantType.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>

std::string osc::doc::to_string(VariantType v)
{
    static auto const s_Lut = osc::to_array<std::string_view>(
    {
        "Bool",
        "Color",
        "Float",
        "Int",
        "String",
        "StringName",
        "Vec3",
    });
    static_assert(std::tuple_size_v<decltype(s_Lut)> == osc::NumOptions<VariantType>());

    auto const idx = std::underlying_type_t<VariantType>(v);
    if (0 <= idx && idx < ssize(s_Lut))
    {
        return std::string{s_Lut[idx]};
    }
    else
    {
        return "Unknown";
    }
}
