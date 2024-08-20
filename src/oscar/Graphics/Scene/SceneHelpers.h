#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/FrustumPlanes.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <functional>
#include <optional>
#include <span>

namespace osc { struct AABB; }
namespace osc { class BVH; }
namespace osc { class Camera; }
namespace osc { class Mesh; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct LineSegment; }
namespace osc { class SceneCache; }
namespace osc { struct Transform; }

namespace osc
{
    void draw_bvh(
        SceneCache&,
        const BVH&,
        const std::function<void(SceneDecoration&&)>& out
    );

    void draw_aabb(
        SceneCache&,
        const AABB&,
        const std::function<void(SceneDecoration&&)>& out
    );

    void draw_aabbs(
        SceneCache&,
        std::span<const AABB>,
        const std::function<void(SceneDecoration&&)>& out
    );

    void draw_bvh_leaf_nodes(
        SceneCache&,
        const BVH&,
        const std::function<void(SceneDecoration&&)>& out
    );

    void draw_xz_floor_lines(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>& out,
        float scale = 5.0f
    );

    void draw_xz_grid(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>& out
    );

    void draw_xy_grid(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>& out
    );

    void draw_yz_grid(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>& out
    );

    struct ArrowProperties final {
        Vec3 start{};
        Vec3 end{};
        float tip_length{};
        float neck_thickness{};
        float head_thickness{};
        Color color = Color::black();
        SceneDecorationFlags decoration_flags = SceneDecorationFlag::Default;
    };
    void draw_arrow(
        SceneCache&,
        const ArrowProperties&,
        const std::function<void(SceneDecoration&&)>& out
    );

    void draw_line_segment(
        SceneCache&,
        const LineSegment&,
        const Color&,
        float radius,
        const std::function<void(SceneDecoration&&)>& out
    );

    AABB worldspace_bounds_of(const SceneDecoration&);

    // updates the given BVH with the given component decorations
    void update_scene_bvh(
        std::span<const SceneDecoration>,
        BVH&
    );

    // returns all collisions along `worldspace_ray`
    std::vector<SceneCollision> get_all_ray_collisions_with_scene(
        const BVH& scene_bvh,
        SceneCache&,
        std::span<const SceneDecoration>,
        const Line& worldspace_ray
    );

    // returns closest ray-triangle collision along `worldspace_ray`
    std::optional<RayCollision> get_closest_worldspace_ray_triangle_collision(
        const Mesh&,
        const BVH& triangle_bvh,
        const Transform&,
        const Line& worldspace_ray
    );

    // returns closest ray-triangle collision in worldspace for a given mouse position
    // within the given render rectangle
    std::optional<RayCollision> get_closest_worldspace_ray_triangle_collision(
        const PolarPerspectiveCamera&,
        const Mesh&,
        const BVH& triangle_bvh,
        const Rect& screen_render_rect,
        Vec2 screen_mouse_pos
    );

    // returns scene rendering parameters for an generic panel
    SceneRendererParams calc_standard_dark_scene_render_params(
        const PolarPerspectiveCamera&,
        AntiAliasingLevel,
        Vec2 render_dims
    );

    // returns a triangle BVH for the given triangle mesh, or an empty BVH if the mesh is non-triangular or empty
    BVH create_triangle_bvh(const Mesh&);

    // returns `FrustumPlanes` that represent the clipping planes of `camera` when rendering to an
    // output that has an aspect ratio of `aspect_ratio`
    FrustumPlanes calc_frustum_planes(const Camera& camera, float aspect_ratio);
}
