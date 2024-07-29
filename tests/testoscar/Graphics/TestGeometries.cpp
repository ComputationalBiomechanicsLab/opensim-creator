#include <oscar/Graphics/Geometries.h>

#include <gtest/gtest.h>
#include <oscar/oscar.h>

using namespace osc;
using namespace osc::literals;

TEST(TorusKnotGeometry, can_default_construct)
{
    const Mesh mesh = TorusKnotGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(TorusKnotGeometry, works_with_non_default_args)
{
    ASSERT_NO_THROW({ TorusKnotGeometry(0.5f, 0.1f, 32, 4, 1, 10); });
    ASSERT_NO_THROW({ TorusKnotGeometry(0.0f, 100.0f, 1, 3, 4, 2); });
}

TEST(BoxGeometry, can_default_construct)
{
    const Mesh mesh = BoxGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(BoxGeometry, works_with_non_default_args)
{
    ASSERT_NO_THROW({ BoxGeometry(0.5f, 100.0f, 0.0f, 10, 1, 5); });
}

TEST(PolyhedronGeometry, works_with_a_couple_of_basic_verts)
{
    const Mesh mesh = PolyhedronGeometry{
        {{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}},
        {{0, 1, 2}},
        5.0f,
        2
    };
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(PolyhedronGeometry, is_empty_if_given_less_than_3_points)
{
    const Mesh mesh = PolyhedronGeometry{
        {{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}}},
        {{0, 1}},
        5.0f,
        2
    };
    ASSERT_FALSE(mesh.has_vertices());
    ASSERT_FALSE(mesh.has_normals());
    ASSERT_FALSE(mesh.has_tex_coords());
    ASSERT_TRUE(mesh.indices().empty());
}


TEST(IcosahedronGeometry, can_default_construct)
{
    const Mesh mesh = IcosahedronGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(IcosahedronGeometry, works_with_non_default_args)
{
    ASSERT_NO_THROW(IcosahedronGeometry(10.0f, 2));
}

TEST(DodecahedronGeometry, can_default_construct)
{
    const Mesh mesh = DodecahedronGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(DodecahedronGeometry, works_with_non_default_args)
{
    const Mesh mesh = DodecahedronGeometry{5.0f, 3};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(OctahedronGeometry, can_default_construct)
{
    const Mesh mesh = OctahedronGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(OctahedronGeometry, works_with_non_default_args)
{
    const Mesh mesh = OctahedronGeometry{11.0f, 2};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(TetrahedronGeometry, can_default_construct)
{
    const Mesh mesh = TetrahedronGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(TetrahedronGeometry, works_with_non_default_args)
{
    const Mesh mesh = TetrahedronGeometry{0.5f, 3};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(LatheGeometry, can_default_construct)
{
    const Mesh mesh = LatheGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(LatheGeometry, works_with_non_default_args)
{
    const Mesh mesh = LatheGeometry{
        {{{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 2.0f}, {3.0f, 3.0f}}},
        32,
        45_deg,
        180_deg
    };
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(CircleGeometry, can_default_construct)
{
    const Mesh mesh = CircleGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(CircleGeometry, works_with_non_default_args)
{
    const Mesh mesh = CircleGeometry{0.5f, 64, 90_deg, 80_deg};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(RingGeometry, can_default_construct)
{
    const Mesh mesh = RingGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(RingGeometry, works_with_non_default_args)
{
    const Mesh mesh = RingGeometry{0.1f, 0.2f, 16, 3, 90_deg, 180_deg};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(TorusGeometry, can_default_construct)
{
    const Mesh mesh = TorusGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(TorusGeometry, works_with_non_default_args)
{
    const Mesh mesh = TorusGeometry{0.2f, 0.3f, 4, 32, 180_deg};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(CylinderGeometry, can_default_construct)
{
    const Mesh mesh = CylinderGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(CylinderGeometry, works_with_non_default_args)
{
    const Mesh mesh = CylinderGeometry{0.1f, 0.05f, 0.5f, 16, 2, true, 180_deg, 270_deg};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(ConeGeometry, can_default_construct)
{
    const Mesh mesh = ConeGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(ConeGeometry, works_with_non_default_args)
{
    const Mesh mesh = ConeGeometry{0.2f, 500.0f, 4, 3, true, -90_deg, 90_deg};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(PlaneGeometry, can_default_construct)
{
    const Mesh mesh = PlaneGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(PlaneGeometry, works_with_non_default_args)
{
    const Mesh mesh = PlaneGeometry{0.5f, 2.0f, 4, 4};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(SphereGeometry, can_default_construct)
{
    const Mesh mesh = SphereGeometry{};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(SphereGeometry, works_with_non_default_args)
{
    const Mesh mesh = SphereGeometry{0.5f, 12, 4, 90_deg, 180_deg, -45_deg, -60_deg};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}
