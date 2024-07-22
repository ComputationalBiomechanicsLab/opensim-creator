#pragma once

#include <iosfwd>
#include <string>

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
    std::string to_string(VariantType);
}
