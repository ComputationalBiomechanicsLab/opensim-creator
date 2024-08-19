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
        Vec2,
        Vec3,
        NUM_OPTIONS
    };

    std::ostream& operator<<(std::ostream&, VariantType);
}
