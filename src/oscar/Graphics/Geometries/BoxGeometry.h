#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    // generates a rectangular cuboid with the given dimensions centered on the origin, with each
    // edge parallel to each axis
    //
    // `segments` affects how many 2-triangle quads may be generated along each dimension
    //
    // inspired by three.js's `BoxGeometry`
    class BoxGeometry final {
    public:
        static Mesh generate_mesh(
            float width = 1.0f,
            float height = 1.0f,
            float depth = 1.0f,
            size_t widthSegments = 1,
            size_t heightSegments = 1,
            size_t depthSegments = 1
        );
    };
}
