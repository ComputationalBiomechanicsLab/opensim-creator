#include "AABBGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Vec3.h>

using namespace osc;

Mesh osc::AABBGeometry::generate_mesh(AABB const& aabb)
{
    auto [min, max] = aabb;

    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts({
        {max.x, max.y, max.z},
        {min.x, max.y, max.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, max.y, min.z},
        {min.x, max.y, min.z},
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
    });
    rv.setIndices({0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7});
    return rv;
}
