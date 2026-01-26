#include "circle_geometry.h"

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/trigonometric_functions.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/assertions.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

using namespace osc;

osc::CircleGeometry::CircleGeometry(const Params& p)
{
    // the implementation of this was initially translated from `three.js`'s
    // `CircleGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/CircleGeometry

    const size_t num_segments = max(3uz, p.num_segments);
    const auto fnum_segments = static_cast<float>(num_segments);

    std::vector<uint32_t> indices;
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<Vector2> uvs;

    // middle vertex
    vertices.emplace_back(0.0f, 0.0f, 0.0f);
    normals.emplace_back(0.0f, 0.0f, 1.0f);
    uvs.emplace_back(0.5f, 0.5f);

    // not-middle vertices
    for (size_t s = 0; s <= num_segments; ++s) {
        const auto fs = static_cast<float>(s);
        const auto segment = p.theta_start + (fs/fnum_segments * p.theta_length);
        const auto cos_segment = cos(segment);
        const auto sin_segment = sin(segment);

        vertices.emplace_back(p.radius * cos_segment, p.radius * sin_segment, 0.0f);
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
