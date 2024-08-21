#pragma once

#include <oscar/Graphics/Snorm.h>

#include <cstdint>

namespace osc
{
    // A normalized signed 8-bit integer that can be used to store a floating-point
    // number in the (clamped) range [-1.0f, 1.0f]
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer
    using Snorm8 = Snorm<int8_t>;
}
