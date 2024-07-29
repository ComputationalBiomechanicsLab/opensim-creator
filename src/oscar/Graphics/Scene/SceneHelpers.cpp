#include "SceneHelpers.h"

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/LineSegment.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PlaneFunctions.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Algorithms.h>

#include <functional>
#include <optional>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    void draw_grid(
        SceneCache& cache,
        const Quat& rotation,
        const std::function<void(SceneDecoration&&)>& out)
    {
        out({
            .mesh = cache.grid_mesh(),
            .transform = {
                .scale = Vec3{50.0f, 50.0f, 1.0f},
                .rotation = rotation,
            },
            .color = {0.7f, 0.15f},
        });
    }
}

void osc::draw_bvh(
    SceneCache& cache,
    const BVH& scene_bvh,
    const std::function<void(SceneDecoration&&)>& out)
{
    scene_bvh.for_each_leaf_or_inner_node([cube = cache.cube_wireframe_mesh(), &out](const BVHNode& node)
    {
        out({
            .mesh = cube,
            .transform = {
                .scale = half_widths_of(node.bounds()),
                .position = centroid_of(node.bounds()),
            },
            .color = Color::black(),
        });
    });
}

void osc::draw_aabb(
    SceneCache& cache,
    const AABB& aabb,
    const std::function<void(SceneDecoration&&)>& out)
{
    draw_aabbs(cache, {{aabb}}, out);
}

void osc::draw_aabbs(
    SceneCache& cache,
    std::span<const AABB> aabbs,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Mesh cube = cache.cube_wireframe_mesh();
    for (const AABB& aabb : aabbs) {
        out({
            .mesh = cube,
            .transform = {
                .scale = half_widths_of(aabb),
                .position = centroid_of(aabb),
            },
            .color = Color::black(),
        });
    }
}

void osc::draw_bvh_leaf_nodes(
    SceneCache& cache,
    const BVH& bvh,
    const std::function<void(SceneDecoration&&)>& out)
{
    bvh.for_each_leaf_node([&cache, &out](const BVHNode& node)
    {
        draw_aabb(cache, node.bounds(), out);
    });
}

void osc::draw_xz_floor_lines(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out,
    float scale)
{
    const Mesh y_line = cache.yline_mesh();

    // X line
    out({
        .mesh = y_line,
        .transform = {
            .scale = Vec3{scale},
            .rotation = angle_axis(90_deg, Vec3{0.0f, 0.0f, 1.0f}),
        },
        .color = Color::red(),
    });

    // Z line
    out({
        .mesh = y_line,
        .transform = {
            .scale = Vec3{scale},
            .rotation = angle_axis(90_deg, Vec3{1.0f, 0.0f, 0.0f}),
        },
        .color = Color::blue(),
    });
}

void osc::draw_xz_grid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Quat rotation = angle_axis(90_deg, Vec3{1.0f, 0.0f, 0.0f});
    draw_grid(cache, rotation, out);
}

void osc::draw_xy_grid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    draw_grid(cache, identity<Quat>(), out);
}

void osc::draw_yz_grid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Quat rotation = angle_axis(90_deg, Vec3{0.0f, 1.0f, 0.0f});
    draw_grid(cache, rotation, out);
}

void osc::draw_arrow(
    SceneCache& cache,
    const ArrowProperties& props,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Vec3 start_to_end = props.end - props.start;
    const float total_length = length(start_to_end);
    const Vec3 direction = start_to_end/total_length;

    // draw the arrow from tip-to-base, because the neck might be
    // excluded in the case where the total length of the arrow is
    // less than or equal to the desired tip length
    const Vec3 tip_start = props.end - (direction * min(props.tip_length, total_length));

    // emit tip cone
    out({
        .mesh = cache.cone_mesh(),
        .transform = cylinder_to_line_segment_transform({tip_start, props.end}, props.head_thickness),
        .color = props.color,
    });

    // if there's space for it, emit the neck cylinder
    if (total_length > props.tip_length) {
        out({
            .mesh = cache.cylinder_mesh(),
            .transform = cylinder_to_line_segment_transform({props.start, tip_start}, props.neck_thickness),
            .color = props.color,
        });
    }
}

void osc::draw_line_segment(
    SceneCache& cache,
    const LineSegment& line_segment,
    const Color& color,
    float radius,
    const std::function<void(SceneDecoration&&)>& out)
{
    out({
        .mesh = cache.cylinder_mesh(),
        .transform = cylinder_to_line_segment_transform(line_segment, radius),
        .color = color,
    });
}

AABB osc::worldspace_bounds_of(const SceneDecoration& decoration)
{
    return transform_aabb(decoration.transform, decoration.mesh.bounds());
}

void osc::update_scene_bvh(std::span<const SceneDecoration> decorations, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(decorations.size());
    for (const SceneDecoration& decoration : decorations) {
        aabbs.push_back(worldspace_bounds_of(decoration));
    }

    bvh.build_from_aabbs(aabbs);
}

std::vector<SceneCollision> osc::get_all_ray_collisions_with_scene(
    const BVH& scene_bvh,
    SceneCache& cache,
    std::span<const SceneDecoration> decorations,
    const Line& worldspace_ray)
{
    std::vector<SceneCollision> rv;
    scene_bvh.for_each_ray_aabb_collision(worldspace_ray, [&cache, &decorations, &worldspace_ray, &rv](BVHCollision scene_collision)
    {
        // perform ray-triangle intersection tests on the scene collisions
        const SceneDecoration& decoration = at(decorations, scene_collision.id);
        const BVH& decoration_triangle_bvh = cache.get_bvh(decoration.mesh);

        const std::optional<RayCollision> maybe_triangle_collision = get_closest_worldspace_ray_triangle_collision(
            decoration.mesh,
            decoration_triangle_bvh,
            decoration.transform,
            worldspace_ray
        );

        if (maybe_triangle_collision) {
            rv.push_back({
                .decoration_id = decoration.id,
                .decoration_index = static_cast<size_t>(scene_collision.id),
                .worldspace_location = maybe_triangle_collision->position,
                .distance_from_ray_origin = maybe_triangle_collision->distance,
            });
        }
    });
    return rv;
}

std::optional<RayCollision> osc::get_closest_worldspace_ray_triangle_collision(
    const Mesh& mesh,
    const BVH& triangle_bvh,
    const Transform& transform,
    const Line& worldspace_ray)
{
    if (mesh.topology() != MeshTopology::Triangles) {
        return std::nullopt;
    }

    // map the ray into the mesh's modelspace, so that we compute a ray-mesh collision
    const Line modespace_ray = inverse_transform_line(worldspace_ray, transform);

    // then perform a ray-AABB (of triangles) collision
    std::optional<RayCollision> rv;
    triangle_bvh.for_each_ray_aabb_collision(modespace_ray, [&mesh, &transform, &worldspace_ray, &modespace_ray, &rv](BVHCollision modelspace_bvh_collision)
    {
        // then perform a ray-triangle collision
        if (auto modelspace_triangle_collision = find_collision(modespace_ray, mesh.get_triangle_at(modelspace_bvh_collision.id))) {
            // map it back into worldspace and check if it's closer
            const Vec3 worldspace_location = transform * modelspace_triangle_collision->position;
            const float distance = length(worldspace_location - worldspace_ray.origin);

            if (not rv or rv->distance > distance) {
                // if it's closer, update the return value
                rv = RayCollision{distance, worldspace_location};
            }
        }
    });
    return rv;
}

std::optional<RayCollision> osc::get_closest_worldspace_ray_triangle_collision(
    const PolarPerspectiveCamera& camera,
    const Mesh& mesh,
    const BVH& triangle_bvh,
    const Rect& screen_render_rect,
    Vec2 screen_mouse_pos)
{
    const Line ray = camera.unproject_topleft_pos_to_world_ray(
        screen_mouse_pos - screen_render_rect.p1,
        dimensions_of(screen_render_rect)
    );

    return get_closest_worldspace_ray_triangle_collision(
        mesh,
        triangle_bvh,
        identity<Transform>(),
        ray
    );
}

SceneRendererParams osc::calc_standard_dark_scene_render_params(
    const PolarPerspectiveCamera& camera,
    AntiAliasingLevel aa_level,
    Vec2 render_dims)
{
    SceneRendererParams rv;
    rv.dimensions = render_dims;
    rv.antialiasing_level = aa_level;
    rv.draw_mesh_normals = false;
    rv.draw_floor = false;
    rv.view_matrix = camera.view_matrix();
    rv.projection_matrix = camera.projection_matrix(aspect_ratio_of(render_dims));
    rv.view_pos = camera.position();
    rv.light_direction = recommended_light_direction(camera);
    rv.background_color = {0.1f, 1.0f};
    return rv;
}

BVH osc::create_triangle_bvh(const Mesh& mesh)
{
    const auto indices = mesh.indices();

    BVH rv;
    if (indices.empty()) {
        return rv;
    }
    else if (mesh.topology() != MeshTopology::Triangles) {
        return rv;
    }
    else if (indices.is_uint32()) {
        rv.build_from_indexed_triangles(mesh.vertices() , indices.to_uint32_span());
    }
    else {
        rv.build_from_indexed_triangles(mesh.vertices(), indices.to_uint16_span());
    }
    return rv;
}

FrustumPlanes osc::calc_frustum_planes(const Camera& camera, float aspect_ratio)
{
    const Radians fov_y = camera.vertical_fov();
    const float z_near = camera.near_clipping_plane();
    const float z_far = camera.far_clipping_plane();
    const float half_v_size = z_far * tan(fov_y * 0.5f);
    const float half_h_size = half_v_size * aspect_ratio;
    const Vec3 pos = camera.position();
    const Vec3 front = camera.direction();
    const Vec3 up = camera.upwards_direction();
    const Vec3 right = cross(front, up);
    const Vec3 front_mult_near = z_near * front;
    const Vec3 front_mult_far = z_far * front;

    return {
                          // origin              // normal
        to_analytic_plane(pos + front_mult_near, -front                                                   ),  // near
        to_analytic_plane(pos + front_mult_far ,  front                                                   ),  // far
        to_analytic_plane(pos                  , -normalize(cross(front_mult_far - right*half_h_size, up))),  // right
        to_analytic_plane(pos                  , -normalize(cross(up, front_mult_far + right*half_h_size))),  // left
        to_analytic_plane(pos                  , -normalize(cross(right, front_mult_far - up*half_v_size))),  // top
        to_analytic_plane(pos                  , -normalize(cross(front_mult_far + up*half_v_size, right))),  // bottom
    };
}
