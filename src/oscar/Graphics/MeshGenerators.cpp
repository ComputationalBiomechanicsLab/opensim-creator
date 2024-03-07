#include "MeshGenerators.h"

#include <oscar/Graphics/Geometries.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/LineSegment.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TriangleFunctions.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/cstddef.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/At.h>
#include <oscar/Utils/EnumHelpers.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <unordered_set>
#include <vector>

using namespace osc::literals;
using namespace osc;

Mesh osc::GenerateGridLinesMesh(size_t n)
{
    constexpr float z = 0.0f;
    constexpr float min = -1.0f;
    constexpr float max = 1.0f;

    float const stepSize = (max - min) / static_cast<float>(n);

    size_t const nlines = n + 1;

    std::vector<Vec3> vertices;
    vertices.reserve(4 * nlines);
    std::vector<uint32_t> indices;
    indices.reserve(4 * nlines);
    std::vector<Vec3> normals;
    normals.reserve(4 * nlines);
    uint32_t index = 0;

    auto push = [&index, &vertices, &indices, &normals](Vec3 const& pos)
    {
        vertices.push_back(pos);
        indices.push_back(index++);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
    };

    // lines parallel to X axis
    for (size_t i = 0; i < nlines; ++i)
    {
        float const y = min + static_cast<float>(i) * stepSize;

        push({-1.0f, y, z});
        push({+1.0f, y, z});
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < nlines; ++i)
    {
        float const x = min + static_cast<float>(i) * stepSize;

        push({x, -1.0f, z});
        push({x, +1.0f, z});
    }

    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setIndices(indices);
    return rv;
}

Mesh osc::GenerateYToYLineMesh()
{
    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts({{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}});
    rv.setNormals({{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}});  // just give them *something* in-case they are rendered through a shader that requires normals
    rv.setIndices({0, 1});
    return rv;
}

Mesh osc::GenerateCubeLinesMesh()
{
    Vec3 min = {-1.0f, -1.0f, -1.0f};
    Vec3 max = { 1.0f,  1.0f,  1.0f};

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

Mesh osc::GenerateTorusKnotMesh(
    float torusRadius,
    float tubeRadius,
    size_t numTubularSegments,
    size_t numRadialSegments,
    size_t p,
    size_t q)
{
    return TorusKnotGeometry::generate_mesh(
        torusRadius,
        tubeRadius,
        numTubularSegments,
        numRadialSegments,
        p,
        q
    );
}

Mesh osc::GenerateBoxMesh(
    float width,
    float height,
    float depth,
    size_t widthSegments,
    size_t heightSegments,
    size_t depthSegments)
{
    return BoxGeometry::generate_mesh(
        width,
        height,
        depth,
        widthSegments,
        heightSegments,
        depthSegments
    );
}

Mesh osc::GeneratePolyhedronMesh(
    std::span<Vec3 const> vertices,
    std::span<uint32_t const> indices,
    float radius,
    size_t detail)
{
    return PolyhedronGeometry::generate_mesh(
        vertices,
        indices,
        radius,
        detail
    );
}

Mesh osc::GenerateIcosahedronMesh(
    float radius,
    size_t detail)
{
    return IcosahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateDodecahedronMesh(
    float radius,
    size_t detail)
{
    return DodecahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateOctahedronMesh(
    float radius,
    size_t detail)
{
    return OctahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateTetrahedronMesh(
    float radius,
    size_t detail)
{
    return TetrahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateLatheMesh(
    std::span<Vec2 const> points,
    size_t segments,
    Radians phiStart,
    Radians phiLength)
{
    return LatheGeometry::generate_mesh(points, segments, phiStart, phiLength);
}

Mesh osc::GenerateCircleMesh(
    float radius,
    size_t segments,
    Radians thetaStart,
    Radians thetaLength)
{
    return CircleGeometry::generate_mesh(radius, segments, thetaStart, thetaLength);
}

Mesh osc::GenerateRingMesh(
    float innerRadius,
    float outerRadius,
    size_t thetaSegments,
    size_t phiSegments,
    Radians thetaStart,
    Radians thetaLength)
{
    return RingGeometry::generate_mesh(innerRadius, outerRadius, thetaSegments, phiSegments, thetaStart, thetaLength);
}

Mesh osc::GenerateTorusMesh(
    float radius,
    float tube,
    size_t radialSegments,
    size_t tubularSegments,
    Radians arc)
{
    return TorusGeometry::generate_mesh(radius, tube, radialSegments, tubularSegments, arc);
}

Mesh osc::GenerateCylinderMesh(
    float radiusTop,
    float radiusBottom,
    float height,
    size_t radialSegments,
    size_t heightSegments,
    bool openEnded,
    Radians thetaStart,
    Radians thetaLength)
{
    return CylinderGeometry::generate_mesh(radiusTop, radiusBottom, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength);
}

Mesh osc::GenerateConeMesh(
    float radius,
    float height,
    size_t radialSegments,
    size_t heightSegments,
    bool openEnded,
    Radians thetaStart,
    Radians thetaLength)
{
    return ConeGeometry::generate_mesh(
        radius,
        height,
        radialSegments,
        heightSegments,
        openEnded,
        thetaStart,
        thetaLength
    );
}

Mesh osc::GeneratePlaneMesh(
    float width,
    float height,
    size_t widthSegments,
    size_t heightSegments)
{
    return PlaneGeometry::generate_mesh(width, height, widthSegments, heightSegments);
}

Mesh osc::GenerateSphereMesh(
    float radius,
    size_t widthSegments,
    size_t heightSegments,
    Radians phiStart,
    Radians phiLength,
    Radians thetaStart,
    Radians thetaLength)
{
    return SphereGeometry::generate_mesh(
        radius,
        widthSegments,
        heightSegments,
        phiStart,
        phiLength,
        thetaStart,
        thetaLength
    );
}

Mesh osc::GenerateWireframeMesh(Mesh const& mesh)
{
    return WireframeGeometry::generate_mesh(mesh);
}
