#pragma once

#include <string>

namespace osc
{
    enum class VariantType {
        Nil = 0,
        Bool,
        Color,
        Float,
        Int,
        String,
        StringName,
        Vec3,

        NUM_OPTIONS
    };

    std::string to_string(VariantType);
}
