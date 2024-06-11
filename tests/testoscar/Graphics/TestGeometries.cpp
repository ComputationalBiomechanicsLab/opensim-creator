#include <oscar/Graphics/Geometries.h>

#include <gtest/gtest.h>
#include <oscar/oscar.h>

using namespace osc;
using namespace osc::literals;

TEST(GenerateTorusKnotMesh, DefaultCtorWorksFine)
{
    const Mesh m = TorusKnotGeometry{};
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
    const Mesh m = BoxGeometry{};
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
    const Mesh m = PolyhedronGeometry{
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
    const Mesh m = PolyhedronGeometry{
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
    const Mesh m = IcosahedronGeometry{};
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
    const Mesh m = DodecahedronGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateDodecahedronMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = DodecahedronGeometry{5.0f, 3};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateOctahedronMesh, DefaultCtorWorksFine)
{
    const Mesh m = OctahedronGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateOctahedronMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = OctahedronGeometry{11.0f, 2};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTetrahedronMesh, DefaultCtorWorksFine)
{
    const Mesh m = TetrahedronGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTetrahedronMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = TetrahedronGeometry{0.5f, 3};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateLatheMesh, DefaultCtorWorksFine)
{
    const Mesh m = LatheGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateLatheMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = LatheGeometry{
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
    const Mesh m = CircleGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateCircleMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = CircleGeometry{0.5f, 64, 90_deg, 80_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateRingMesh, DefaultCtorWorksFine)
{
    const Mesh m = RingGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateRingMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = RingGeometry{0.1f, 0.2f, 16, 3, 90_deg, 180_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTorusMesh, DefaultCtorWorksFine)
{
    const Mesh m = TorusGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateTorusMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = TorusGeometry{0.2f, 0.3f, 4, 32, 180_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateCylinderMesh, DefaultCtorWorksFine)
{
    const Mesh m = CylinderGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateCylinderMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = CylinderGeometry{0.1f, 0.05f, 0.5f, 16, 2, true, 180_deg, 270_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateConeMesh, DefaultCtorWorksFine)
{
    const Mesh m = ConeGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateConeMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = ConeGeometry{0.2f, 500.0f, 4, 3, true, -90_deg, 90_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GeneratePlaneMesh, DefaultCtorWorksFine)
{
    const Mesh m = PlaneGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GeneratePlaneMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = PlaneGeometry{0.5f, 2.0f, 4, 4};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateSphereMesh, DefaultCtorWorksFine)
{
    const Mesh m = SphereGeometry{};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}

TEST(GenerateSphereMesh, WorksWithNonDefaultArgs)
{
    const Mesh m = SphereGeometry{0.5f, 12, 4, 90_deg, 180_deg, -45_deg, -60_deg};
    ASSERT_TRUE(m.has_vertices());
    ASSERT_TRUE(m.has_normals());
    ASSERT_TRUE(m.has_tex_coords());
    ASSERT_FALSE(m.indices().empty());
}
