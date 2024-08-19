#include "CircleGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/cstddef.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::CircleGeometry::CircleGeometry(
    float radius,
    size_t num_segments,
    Radians theta_start,
    Radians theta_length)
{
    // the implementation of this was initially translated from `three.js`'s
    // `CircleGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/CircleGeometry

    num_segments = max(3_uz, num_segments);
    const auto fnum_segments = static_cast<float>(num_segments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // middle vertex
    vertices.emplace_back(0.0f, 0.0f, 0.0f);
    normals.emplace_back(0.0f, 0.0f, 1.0f);
    uvs.emplace_back(0.5f, 0.5f);

    // not-middle vertices
    for (size_t s = 0; s <= num_segments; ++s) {
        const auto fs = static_cast<float>(s);
        const auto segment = theta_start + (fs/fnum_segments * theta_length);
        const auto cos_segment = cos(segment);
        const auto sin_segment = sin(segment);

        vertices.emplace_back(radius * cos_segment, radius * sin_segment, 0.0f);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
        uvs.emplace_back((cos_segment + 1.0f) / 2.0f, (sin_segment + 1.0f) / 2.0f);
    }

    OSC_ASSERT(num_segments + 1 < std::numeric_limits<uint32_t>::max());
    for (uint32_t i = 1; i <= static_cast<uint32_t>(num_segments); ++i) {
        indices.insert(indices.end(), {i, i+1, 0});
    }

    mesh_.set_vertices(vertices);
    mesh_.set_normals(normals);
    mesh_.set_tex_coords(uvs);
    mesh_.set_indices(indices);
}
