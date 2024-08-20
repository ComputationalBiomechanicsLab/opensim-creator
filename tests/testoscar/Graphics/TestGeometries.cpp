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
    ASSERT_NO_THROW({ TorusKnotGeometry({
        .torus_radius = 0.5f,
        .tube_radius = 0.1f,
        .num_tubular_segments = 32,
        .num_radial_segments = 4,
        .p = 1,
        .q = 10,
    }); });
    ASSERT_NO_THROW({ TorusKnotGeometry({
        .torus_radius = 0.0f,
        .tube_radius = 100.0f,
        .num_tubular_segments = 1,
        .num_radial_segments = 3,
        .p = 4,
        .q = 2,
    }); });
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
    ASSERT_NO_THROW({ BoxGeometry({
        .width = 0.5f,
        .height = 100.0f,
        .depth = 0.0f,
        .num_width_segments = 10,
        .num_height_segments = 1,
        .num_depth_segments = 5,
    }); });
}

TEST(PolyhedronGeometry, can_default_construct_with_params)
{
    PolyhedronGeometry mesh;

    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(PolyhedronGeometry, can_construct_with_custom_params)
{
    PolyhedronGeometry mesh{{
        .vertices = {{1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}},
        .indices = {2, 1, 0,    0, 3, 2,    1, 3, 0,    2, 3, 1},
        .radius = 10.0f,
        .detail_level = 1,
    }};
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
    ASSERT_NO_THROW(IcosahedronGeometry({.radius = 10.0f, .detail = 2}));
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
    const Mesh mesh = DodecahedronGeometry{{.radius = 5.0f, .detail = 3}};
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
    const Mesh mesh = OctahedronGeometry{{.radius = 11.0f, .detail = 2}};
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
    const Mesh mesh = TetrahedronGeometry{{.radius = 0.5f, .detail_level = 3}};
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
    const Mesh mesh = LatheGeometry{{
        .points = {{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 2.0f}, {3.0f, 3.0f}},
        .num_segments = 32,
        .phi_start = 45_deg,
        .phi_length = 180_deg,
    }};
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
    const Mesh mesh = CircleGeometry{{
        .radius = 0.5f,
        .num_segments = 64,
        .theta_start = 90_deg,
        .theta_length = 80_deg,
    }};
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
    const Mesh mesh = RingGeometry{{
        .inner_radius = 0.1f,
        .outer_radius = 0.2f,
        .num_theta_segments = 16,
        .num_phi_segments = 3,
        .theta_start = 90_deg,
        .theta_length = 180_deg,
    }};
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
    const Mesh mesh = TorusGeometry{{
        .tube_center_radius = 0.2f,
        .tube_radius = 0.3f,
        .num_radial_segments = 4,
        .num_tubular_segments = 32,
        .arc = 180_deg,
    }};
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
    const Mesh mesh = CylinderGeometry{{
        .radius_top = 0.1f,
        .radius_bottom = 0.05f,
        .height = 0.5f,
        .num_radial_segments = 16,
        .num_height_segments = 2,
        .open_ended = true,
        .theta_start = 180_deg,
        .theta_length = 270_deg
    }};
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
    const Mesh mesh = ConeGeometry{{
        .radius = 0.2f,
        .height = 500.0f,
        .num_radial_segments = 4,
        .num_height_segments = 3,
        .open_ended = true,
        .theta_start = -90_deg,
        .theta_length = 90_deg,
    }};
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
    const Mesh mesh = PlaneGeometry{{
        .width = 0.5f,
        .height = 2.0f,
        .num_width_segments = 4,
        .num_height_segments = 4
    }};
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
    const Mesh mesh = SphereGeometry{{
        .radius = 0.5f,
        .num_width_segments = 12,
        .num_height_segments = 4,
        .phi_start = 90_deg,
        .phi_length = 180_deg,
        .theta_start = -45_deg,
        .theta_length = -60_deg,
    }};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(WireframeGeometry, can_default_construct)
{
    WireframeGeometry mesh;
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}

TEST(WireframeGeometry, can_construct_from_some_other_geometry)
{
    WireframeGeometry mesh{TorusKnotGeometry{}};
    ASSERT_TRUE(mesh.has_vertices());
    ASSERT_TRUE(mesh.has_normals());
    ASSERT_TRUE(mesh.has_tex_coords());
    ASSERT_FALSE(mesh.indices().empty());
}
