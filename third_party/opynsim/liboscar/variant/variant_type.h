#pragma once

#include <iosfwd>

namespace osc
{
    enum class VariantType {
        None,
        Bool,
        Color,
        Float,
        Int,
        String,
        StringName,
        Vector2,
        Vector3,
        VariantArray,
        NUM_OPTIONS
    };

    std::ostream& operator<<(std::ostream&, VariantType);
}
