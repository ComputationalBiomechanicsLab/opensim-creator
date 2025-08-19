#pragma once

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/Scene/SceneCollision.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/BVH.h>
#include <liboscar/Maths/FrustumPlanes.h>
#include <liboscar/Maths/Ray.h>
#include <liboscar/Maths/RayCollision.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>

#include <functional>
#include <optional>
#include <span>

namespace osc { class BVH; }
namespace osc { class Camera; }
namespace osc { class Mesh; }
namespace osc { class Rect; }
namespace osc { class SceneCache; }
namespace osc { struct AABB; }
namespace osc { struct LineSegment; }
namespace osc { struct PolarPerspectiveCamera; }
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
        Vector3 start{};
        Vector3 end{};
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

    AABB world_space_bounds_of(const SceneDecoration&);

    // updates the given BVH with the given component decorations
    void update_scene_bvh(
        std::span<const SceneDecoration>,
        BVH&
    );

    // calls `out` with each `SceneCollision` found along `world_space_ray`
    void for_each_ray_collision_with_scene(
        const BVH& scene_bvh,
        SceneCache&,
        std::span<const SceneDecoration>,
        const Ray& world_space_ray,
        const std::function<void(SceneCollision&&)>& out
    );

    // returns all collisions along `world_space_ray`
    std::vector<SceneCollision> get_all_ray_collisions_with_scene(
        const BVH& scene_bvh,
        SceneCache&,
        std::span<const SceneDecoration>,
        const Ray& world_space_ray
    );

    // returns closest ray-triangle collision along `world_space_ray`
    std::optional<RayCollision> get_closest_world_space_ray_triangle_collision(
        const Mesh&,
        const BVH& triangle_bvh,
        const Transform&,
        const Ray& world_space_ray
    );

    // returns closest ray-triangle collision in world space for a given mouse position
    // within the given render rectangle
    std::optional<RayCollision> get_closest_world_space_ray_triangle_collision(
        const PolarPerspectiveCamera&,
        const Mesh&,
        const BVH& triangle_bvh,
        const Rect& screen_render_rect,
        Vector2 mouse_screen_position
    );

    // returns scene rendering parameters for an generic panel
    SceneRendererParams calc_standard_dark_scene_render_params(
        const PolarPerspectiveCamera&,
        AntiAliasingLevel,
        Vector2 dimensions,
        float device_pixel_ratio
    );

    // returns a triangle BVH for the given triangle mesh, or an empty BVH if the mesh is non-triangular or empty
    BVH create_triangle_bvh(const Mesh&);

    // returns `FrustumPlanes` that represent the clipping planes of `camera` when rendering to an
    // output that has an aspect ratio of `aspect_ratio`
    FrustumPlanes calc_frustum_planes(const Camera& camera, float aspect_ratio);
}
