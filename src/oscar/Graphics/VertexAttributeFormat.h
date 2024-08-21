#pragma once

namespace osc
{
    enum class VertexAttributeFormat {
        // two single-precision floats
        Float32x2,

        // three single-precision floats
        Float32x3,

        // four single-precision floats
        Float32x4,

        // four unsigned bytes, [0, 255], converted to floats [0.0, 1.0] when loaded into a shader
        Unorm8x4,

        // four signed bytes, [-128, 127], converted to floats [-1.0f, 1.0f] when loaded into a shader
        Snorm8x4,

        NUM_OPTIONS,
    };
}
