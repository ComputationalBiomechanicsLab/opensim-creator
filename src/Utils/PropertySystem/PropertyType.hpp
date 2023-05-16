#pragma once

#include <cstddef>

namespace osc
{
    enum class PropertyType {
        Float = 0,
        Vec3,
        String,

        TOTAL,
    };

    static constexpr size_t NumPropertyTypes() noexcept
    {
        return static_cast<size_t>(PropertyType::TOTAL);
    }
}