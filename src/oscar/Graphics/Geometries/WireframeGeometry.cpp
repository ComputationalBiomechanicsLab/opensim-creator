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

osc::WireframeGeometry::WireframeGeometry(Mesh const& mesh)
{
    static_assert(NumOptions<MeshTopology>() == 2);

    if (mesh.getTopology() == MeshTopology::Lines) {
        m_Mesh = mesh;
        return;
    }

    std::unordered_set<LineSegment> edges;
    edges.reserve(mesh.getNumIndices());  // (guess)

    std::vector<Vec3> points;
    points.reserve(mesh.getNumIndices());  // (guess)

    mesh.forEachIndexedTriangle([&edges, &points](Triangle const& triangle)
    {
        auto [a, b, c] = triangle;

        auto const orderedEdge = [](Vec3 p1, Vec3 p2)
        {
            return lexicographical_compare(p1, p2) ? LineSegment{p1, p2} : LineSegment{p2, p1};
        };

        if (auto ab = orderedEdge(a, b); edges.emplace(ab).second) {
            points.insert(points.end(), {ab.start, ab.end});
        }

        if (auto ac = orderedEdge(a, c); edges.emplace(ac).second) {
            points.insert(points.end(), {ac.start, ac.end});
        }

        if (auto bc = orderedEdge(b, c); edges.emplace(bc).second) {
            points.insert(points.end(), {bc.start, bc.end});
        }
    });

    std::vector<uint32_t> indices;
    indices.reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        indices.push_back(static_cast<uint32_t>(i));
    }

    m_Mesh.setTopology(MeshTopology::Lines);
    m_Mesh.setVerts(points);
    m_Mesh.setIndices(indices);
}
