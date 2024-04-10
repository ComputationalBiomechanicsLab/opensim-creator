#include "WireframeGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/LineSegment.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

using namespace osc;

osc::WireframeGeometry::WireframeGeometry(const Mesh& mesh)
{
    static_assert(num_options<MeshTopology>() == 2);

    if (mesh.topology() == MeshTopology::Lines) {
        mesh_ = mesh;
        return;
    }

    std::unordered_set<LineSegment> edges;
    edges.reserve(mesh.num_indices());  // (guess)

    std::vector<Vec3> vertices;
    vertices.reserve(mesh.num_indices());  // (guess)

    mesh.for_each_indexed_triangle([&edges, &vertices](const Triangle& triangle)
    {
        const auto [a, b, c] = triangle;

        const auto ordered_edge = [](Vec3 p1, Vec3 p2)
        {
            return lexicographical_compare(p1, p2) ? LineSegment{p1, p2} : LineSegment{p2, p1};
        };

        if (auto ab = ordered_edge(a, b); edges.emplace(ab).second) {
            vertices.insert(vertices.end(), {ab.start, ab.end});
        }

        if (auto ac = ordered_edge(a, c); edges.emplace(ac).second) {
            vertices.insert(vertices.end(), {ac.start, ac.end});
        }

        if (auto bc = ordered_edge(b, c); edges.emplace(bc).second) {
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
