#include <oscar/Graphics/Geometries.h>

#include <gtest/gtest.h>
#include <oscar/oscar.h>

using namespace osc;
using namespace osc::literals;

TEST(GenerateTorusKnotMesh, DefaultCtorWorksFine)
{
    Mesh const m = TorusKnotGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTorusKnotMesh, WorksWithOtherArguments)
{
    ASSERT_NO_THROW({ TorusKnotGeometry(0.5f, 0.1f, 32, 4, 1, 10); });
    ASSERT_NO_THROW({ TorusKnotGeometry(0.0f, 100.0f, 1, 3, 4, 2); });
}

TEST(GenerateBoxMesh, DefaultCtorWorksFine)
{
    Mesh const m = BoxGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateBoxMesh, WorksWithNonDefaultArgs)
{
    ASSERT_NO_THROW({ BoxGeometry(0.5f, 100.0f, 0.0f, 10, 1, 5); });
}

TEST(GeneratePolyhedronMesh, WorksWithACoupleOfBasicVerts)
{
    Mesh const m = PolyhedronGeometry{
        {{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}},
        {{0, 1, 2}},
        5.0f,
        2
    };
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GeneratePolyhedronMesh, ReturnsEmptyMeshIfGivenLessThanThreePoints)
{
    Mesh const m = PolyhedronGeometry{
        {{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}}},
        {{0, 1}},
        5.0f,
        2
    };
    ASSERT_FALSE(m.has_vertices());
    ASSERT_FALSE(m.has_normals());
    ASSERT_FALSE(m.has_tex_coords());
    ASSERT_TRUE(m.indices().empty());
}


TEST(GenerateIcosahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = IcosahedronGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateIcosahedronMesh, WorksWithNonDefaultArgs)
{
    ASSERT_NO_THROW(IcosahedronGeometry(10.0f, 2));
}

TEST(GenerateDodecahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = DodecahedronGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateDodecahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = DodecahedronGeometry{5.0f, 3};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateOctahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = OctahedronGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateOctahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = OctahedronGeometry{11.0f, 2};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTetrahedronMesh, DefaultCtorWorksFine)
{
    Mesh const m = TetrahedronGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTetrahedronMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = TetrahedronGeometry{0.5f, 3};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateLatheMesh, DefaultCtorWorksFine)
{
    Mesh const m = LatheGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateLatheMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = LatheGeometry{
        {{{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 2.0f}, {3.0f, 3.0f}}},
        32,
        45_deg,
        180_deg
    };
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateCircleMesh, DefaultCtorWorksFine)
{
    Mesh const m = CircleGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateCircleMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = CircleGeometry{0.5f, 64, 90_deg, 80_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateRingMesh, DefaultCtorWorksFine)
{
    Mesh const m = RingGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateRingMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = RingGeometry{0.1f, 0.2f, 16, 3, 90_deg, 180_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTorusMesh, DefaultCtorWorksFine)
{
    Mesh const m = TorusGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTorusMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = TorusGeometry{0.2f, 0.3f, 4, 32, 180_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateCylinderMesh, DefaultCtorWorksFine)
{
    Mesh const m = CylinderGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateCylinderMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = CylinderGeometry{0.1f, 0.05f, 0.5f, 16, 2, true, 180_deg, 270_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateConeMesh, DefaultCtorWorksFine)
{
    Mesh const m = ConeGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateConeMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = ConeGeometry{0.2f, 500.0f, 4, 3, true, -90_deg, 90_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GeneratePlaneMesh, DefaultCtorWorksFine)
{
    Mesh const m = PlaneGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GeneratePlaneMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = PlaneGeometry{0.5f, 2.0f, 4, 4};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateSphereMesh, DefaultCtorWorksFine)
{
    Mesh const m = SphereGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateSphereMesh, WorksWithNonDefaultArgs)
{
    Mesh const m = SphereGeometry{0.5f, 12, 4, 90_deg, 180_deg, -45_deg, -60_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}
