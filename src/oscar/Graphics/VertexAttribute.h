#pragma once

namespace osc
{
    // denotes which vertex attribute a VertexAttributeDescriptor is describing
    //
    // the order of elements in this enum matches the order of data within a
    // vertex buffer
    enum class VertexAttribute {
        Position,
        Normal,
        Tangent,
        Color,
        TexCoord0,
        NUM_OPTIONS,
    };
}
