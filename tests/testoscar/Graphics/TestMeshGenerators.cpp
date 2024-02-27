#include <oscar/Graphics/MeshGenerators.h>

#include <gtest/gtest.h>
#include <oscar/oscar.h>

using namespace osc;

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
