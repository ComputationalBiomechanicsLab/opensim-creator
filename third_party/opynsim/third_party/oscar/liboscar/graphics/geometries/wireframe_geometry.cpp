#include "wireframe_geometry.h"

#include <liboscar/graphics/geometries/box_geometry.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/mesh_topology.h>
#include <liboscar/maths/common_functions.h>
#include <liboscar/maths/line_segment.h>
#include <liboscar/maths/triangle.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utils/enum_helpers.h>

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

osc::WireframeGeometry::WireframeGeometry() :
    WireframeGeometry{BoxGeometry{}}
{}

osc::WireframeGeometry::WireframeGeometry(const Mesh& mesh)
{
    // the implementation/API of this was initially translated from `three.js`'s
    // `WireframeGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/WireframeGeometry

    static_assert(num_options<MeshTopology>() == 2);

    if (mesh.topology() == MeshTopology::Lines) {
        mesh_ = mesh;
        return;
    }

    std::unordered_set<LineSegment> edges;
    edges.reserve(mesh.num_indices());  // (guess)

    std::vector<Vector3> vertices;
    vertices.reserve(mesh.num_indices());  // (guess)

    mesh.for_each_indexed_triangle([&edges, &vertices](const Triangle& triangle)
    {
        const auto [a, b, c] = triangle;

        const auto ordered_edge = [](Vector3 p1, Vector3 p2)
        {
            return rgs::lexicographical_compare(p1, p2) ? LineSegment{p1, p2} : LineSegment{p2, p1};
        };

        if (const auto ab = ordered_edge(a, b); edges.emplace(ab).second) {
            vertices.insert(vertices.end(), {ab.start, ab.end});
        }

        if (const auto ac = ordered_edge(a, c); edges.emplace(ac).second) {
            vertices.insert(vertices.end(), {ac.start, ac.end});
        }

        if (const auto bc = ordered_edge(b, c); edges.emplace(bc).second) {
            vertices.insert(vertices.end(), {bc.start, bc.end});
        }
    });

    std::vector<uint32_t> indices;
    indices.reserve(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        indices.push_back(static_cast<uint32_t>(i));
    }

    mesh_.set_topology(MeshTopology::Lines);
    mesh_.set_vertices(vertices);
    mesh_.set_indices(indices);
}
