#pragma once

#include <cstdint>

namespace osc
{
    enum class VertexAttributeFormat : int8_t {
        // two single-precision floats
        Float32x2,

        // three single-precision floats
        Float32x3,

        // four single-precision floats
        Float32x4,

        // four unsigned bytes, [0, 255], converted to float [0.0, 1.0] when loaded into a shader
        Unorm8x4,

        NUM_OPTIONS,
    };
}
