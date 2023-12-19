#pragma once

namespace osc
{
    enum class VertexAttributeFormat {
        // 32-bit float
        Float32,

        // 8-bit unsigned normalized number ([0, 1] - handy for LDR colors)
        UNorm8,

        NUM_OPTIONS,
    };
}
