#pragma once

#include <string>

namespace osc::doc
{
    enum class VariantType {
        Bool = 0,
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
