#include <oscar/Graphics/Geometries.h>

#include <gtest/gtest.h>
#include <oscar/oscar.h>

using namespace osc;
using namespace osc::literals;

TEST(GenerateTorusKnotMesh, DefaultCtorWorksFine)
{
    Mesh const m = TorusKnotGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTorusKnotMesh, WorksWithOtherArguments)
{
    ASSERT_NO_THROW({ TorusKnotGeometry::generate_mesh(0.5f, 0.1f, 32, 4, 1, 10); });
    ASSERT_NO_THROW({ TorusKnotGeometry::generate_mesh(0.0f, 100.0f, 1, 3, 4, 2); });
}

TEST(GenerateBoxMesh, DefaultCtorWorksFine)
{
    Mesh const m = BoxGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateBoxMesh, WorksWithNonDefaultArgs)
{
    ASSERT_NO_THROW({ BoxGeometry::generate_mesh(0.5f, 100.0f, 0.0f, 10, 1, 5); });
}

TEST(GeneratePolyhedronMesh, WorksWithACoupleOfBasicVerts)
{
    Mesh const m = PolyhedronGeometry::generate_mesh(
        {{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}},
        {{0, 1, 2}},
        5.0f,
        2
    );
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GeneratePolyhedronMesh, ReturnsEmptyMeshIfGivenLessThanThreePoints)
{
    Mesh const m = PolyhedronGeometry::generate_mesh(
        {{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}}},
        {{0, 1}},
        5.0f,
        2
    );
    ASSERT_FALSE(m.hasVerts());
    ASSERT_FALSE(m.hasNormals());
    ASSERT_FALSE(m.hasTexCoords());
    ASSERT_TRUE(m.getIndices().empty());
}


TEST(GenerateIcosahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = IcosahedronGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateIcosahedronMesh, WorksWithNonDefaultArgs)
{
    ASSERT_NO_THROW(IcosahedronGeometry::generate_mesh(10.0f, 2));
}

TEST(GenerateDodecahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = DodecahedronGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateDodecahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = DodecahedronGeometry::generate_mesh(5.0f, 3);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateOctahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = OctahedronGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateOctahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = OctahedronGeometry::generate_mesh(11.0f, 2);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTetrahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = TetrahedronGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTetrahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = TetrahedronGeometry::generate_mesh(0.5f, 3);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateLatheMesh, DefaultCtorWorksFine)
{
    Mesh const m = LatheGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateLatheMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = LatheGeometry::generate_mesh(
        {{{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 2.0f}, {3.0f, 3.0f}}},
        32,
        45_deg,
        180_deg
    );
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateCircleMesh, DefaultCtorWorksFine)
{
    Mesh const m = CircleGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateCircleMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = CircleGeometry::generate_mesh(0.5f, 64, 90_deg, 80_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateRingMesh, DefaultCtorWorksFine)
{
    Mesh const m = RingGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateRingMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = RingGeometry::generate_mesh(0.1f, 0.2f, 16, 3, 90_deg, 180_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTorusMesh, DefaultCtorWorksFine)
{
    Mesh const m = TorusGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTorusMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = TorusGeometry::generate_mesh(0.2f, 0.3f, 4, 32, 180_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateCylinderMesh, DefaultCtorWorksFine)
{
    Mesh const m = CylinderGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateCylinderMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = CylinderGeometry::generate_mesh(0.1f, 0.05f, 0.5f, 16, 2, true, 180_deg, 270_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateConeMesh, DefaultCtorWorksFine)
{
    Mesh const m = ConeGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateConeMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = ConeGeometry::generate_mesh(0.2f, 500.0f, 4, 3, true, -90_deg, 90_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GeneratePlaneMesh, DefaultCtorWorksFine)
{
    Mesh const m = PlaneGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GeneratePlaneMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = PlaneGeometry::generate_mesh(0.5f, 2.0f, 4, 4);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateSphereMesh, DefaultCtorWorksFine)
{
    Mesh const m = SphereGeometry::generate_mesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateSphereMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = SphereGeometry::generate_mesh(0.5f, 12, 4, 90_deg, 180_deg, -45_deg, -60_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}
