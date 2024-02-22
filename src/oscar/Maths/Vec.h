#pragma once

#include <cstddef>

namespace osc
{
    // top-level template that's specialized by `Vec2`, `Vec3`, and `Vec4`
    template<size_t L, typename T>
    struct Vec;
}
