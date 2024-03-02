#include <oscar/Graphics/MeshGenerators.h>

#include <gtest/gtest.h>
#include <oscar/oscar.h>

using namespace osc;
using namespace osc::literals;

TEST(GenerateTorusKnotMesh, DefaultCtorWorksFine)
{
    Mesh const m = GenerateTorusKnotMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTorusKnotMesh, WorksWithOtherArguments)
{
    ASSERT_NO_THROW({ GenerateTorusKnotMesh(0.5f, 0.1f, 32, 4, 1, 10); });
    ASSERT_NO_THROW({ GenerateTorusKnotMesh(0.0f, 100.0f, 1, 3, 4, 2); });
}

TEST(GenerateBoxMesh, DefaultCtorWorksFine)
{
    Mesh const m = GenerateBoxMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateBoxMesh, WorksWithNonDefaultArgs)
{
    ASSERT_NO_THROW({ GenerateBoxMesh(0.5f, 100.0f, 0.0f, 10, 1, 5); });
}

TEST(GeneratePolyhedronMesh, WorksWithACoupleOfBasicVerts)
{
    Mesh const m = GeneratePolyhedronMesh(
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
    Mesh const m = GeneratePolyhedronMesh(
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
    Mesh const m = GenerateIcosahedronMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateIcosahedronMesh, WorksWithNonDefaultArgs)
{
    ASSERT_NO_THROW(GenerateIcosahedronMesh(10.0f, 2));
}

TEST(GenerateDodecahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = GenerateDodecahedronMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateDodecahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = GenerateDodecahedronMesh(5.0f, 3);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateOctahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = GenerateOctahedronMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateOctahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = GenerateOctahedronMesh(11.0f, 2);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTetrahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = GenerateTetrahedronMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTetrahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = GenerateTetrahedronMesh(0.5f, 3);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateLatheMesh, DefaultCtorWorksFine)
{
    Mesh const m = GenerateLatheMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateLatheMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = GenerateLatheMesh(
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

TEST(GenerateCircleMesh2, DefaultCtorWorksFine)
{
    Mesh const m = GenerateCircleMesh2();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateCircleMesh2, WorksWithNonDefaultArgs)
{
    Mesh const m = GenerateCircleMesh2(0.5f, 64, 90_deg, 80_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateRingMesh, DefaultCtorWorksFine)
{
    Mesh const m = GenerateRingMesh();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateRingMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = GenerateRingMesh(0.1f, 0.2f, 16, 3, 90_deg, 180_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTorusMesh2, DefaultCtorWorksFine)
{
    Mesh const m = GenerateTorusMesh2();
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}

TEST(GenerateTorusMesh2, WorksWithNonDefaultArgs)
{
    Mesh const m = GenerateTorusMesh2(0.2f, 0.3f, 4, 32, 180_deg);
    ASSERT_TRUE(m.hasVerts());
    ASSERT_TRUE(m.hasNormals());
    ASSERT_TRUE(m.hasTexCoords());
    ASSERT_FALSE(m.getIndices().empty());
}
