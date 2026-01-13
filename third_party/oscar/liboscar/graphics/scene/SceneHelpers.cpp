#include "SceneHelpers.h"

#include <liboscar/graphics/AntiAliasingLevel.h>
#include <liboscar/graphics/Camera.h>
#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/MeshIndicesView.h>
#include <liboscar/graphics/MeshTopology.h>
#include <liboscar/graphics/scene/SceneCache.h>
#include <liboscar/graphics/scene/SceneDecoration.h>
#include <liboscar/graphics/scene/SceneRendererParams.h>
#include <liboscar/maths/AABB.h>
#include <liboscar/maths/Angle.h>
#include <liboscar/maths/BVH.h>
#include <liboscar/maths/CollisionTests.h>
#include <liboscar/maths/GeometricFunctions.h>
#include <liboscar/maths/LineSegment.h>
#include <liboscar/maths/MathHelpers.h>
#include <liboscar/maths/PlaneFunctions.h>
#include <liboscar/maths/PolarPerspectiveCamera.h>
#include <liboscar/maths/Quaternion.h>
#include <liboscar/maths/Ray.h>
#include <liboscar/maths/RayCollision.h>
#include <liboscar/maths/Rect.h>
#include <liboscar/maths/Transform.h>
#include <liboscar/maths/TrigonometricFunctions.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/utils/Algorithms.h>

#include <functional>
#include <optional>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    void draw_grid(
        SceneCache& cache,
        const Quaternion& rotation,
        const std::function<void(SceneDecoration&&)>& out)
    {
        out(SceneDecoration{
            .mesh = cache.grid_mesh(),
            .transform = {
                .scale = Vector3{50.0f, 50.0f, 1.0f},
                .rotation = rotation,
            },
            .shading = Color::light_grey().with_alpha(0.15f),
            .flags = SceneDecorationFlag::AnnotationElement,
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
        out(SceneDecoration{
            .mesh = cube,
            .transform = {
                .scale = half_widths_of(node.bounds()),
                .translation = centroid_of(node.bounds()),
            },
            .shading = Color::black(),
            .flags = SceneDecorationFlag::AnnotationElement,
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
        out(SceneDecoration{
            .mesh = cube,
            .transform = {
                .scale = half_widths_of(aabb),
                .translation = centroid_of(aabb),
            },
            .shading = Color::black(),
            .flags = SceneDecorationFlag::AnnotationElement,
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
    out(SceneDecoration{
        .mesh = y_line,
        .transform = {
            .scale = Vector3{scale},
            .rotation = angle_axis(90_deg, Vector3{0.0f, 0.0f, 1.0f}),
        },
        .shading = Color::red(),
        .flags = SceneDecorationFlag::AnnotationElement,
    });

    // Z line
    out(SceneDecoration{
        .mesh = y_line,
        .transform = {
            .scale = Vector3{scale},
            .rotation = angle_axis(90_deg, Vector3{1.0f, 0.0f, 0.0f}),
        },
        .shading = Color::blue(),
        .flags = SceneDecorationFlag::AnnotationElement,
    });
}

void osc::draw_xz_grid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Quaternion rotation = angle_axis(90_deg, Vector3{1.0f, 0.0f, 0.0f});
    draw_grid(cache, rotation, out);
}

void osc::draw_xy_grid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    draw_grid(cache, identity<Quaternion>(), out);
}

void osc::draw_yz_grid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Quaternion rotation = angle_axis(90_deg, Vector3{0.0f, 1.0f, 0.0f});
    draw_grid(cache, rotation, out);
}

void osc::draw_arrow(
    SceneCache& cache,
    const ArrowProperties& props,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Vector3 start_to_end = props.end - props.start;
    const float total_length = length(start_to_end);
    if (isnan(total_length) or equal_within_epsilon(total_length, 0.0f)) {
        return;  // edge-case: caller passed junk vectors to this implementation
    }
    const Vector3 direction = start_to_end/total_length;

    // draw the arrow from tip-to-base, because the neck might be
    // excluded in the case where the total length of the arrow is
    // less than or equal to the desired tip length
    const Vector3 tip_start = props.end - (direction * min(props.tip_length, total_length));

    // emit tip cone
    out(SceneDecoration{
        .mesh = cache.cone_mesh(),
        .transform = cylinder_to_line_segment_transform({tip_start, props.end}, props.head_thickness),
        .shading = props.color,
        .flags = props.decoration_flags,
    });

    // if there's space for it, emit the neck cylinder
    if (total_length > props.tip_length) {
        out(SceneDecoration{
            .mesh = cache.cylinder_mesh(),
            .transform = cylinder_to_line_segment_transform({props.start, tip_start}, props.neck_thickness),
            .shading = props.color,
            .flags = props.decoration_flags,
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
    out(SceneDecoration{
        .mesh = cache.cylinder_mesh(),
        .transform = cylinder_to_line_segment_transform(line_segment, radius),
        .shading = color,
    });
}

void osc::update_scene_bvh(std::span<const SceneDecoration> decorations, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(decorations.size());
    for (const SceneDecoration& decoration : decorations) {
        if (const std::optional<AABB> bounds = decoration.world_space_bounds()) {
            aabbs.push_back(*bounds);
        }
    }

    bvh.build_from_aabbs(aabbs);
}

void osc::for_each_ray_collision_with_scene(
    const BVH& scene_bvh,
    SceneCache& cache,
    std::span<const SceneDecoration> decorations,
    const Ray& world_space_ray,
    const std::function<void(SceneCollision&&)>& out)
{
    scene_bvh.for_each_ray_aabb_collision(world_space_ray, [&cache, &decorations, &world_space_ray, &out](const BVHCollision& scene_collision)
    {
        // perform ray-triangle intersection tests on the scene collisions
        const SceneDecoration& decoration = at(decorations, scene_collision.id);
        const BVH& decoration_triangle_bvh = cache.get_bvh(decoration.mesh);

        const std::optional<RayCollision> maybe_triangle_collision = get_closest_world_space_ray_triangle_collision(
            decoration.mesh,
            decoration_triangle_bvh,
            decoration.transform,
            world_space_ray
        );

        if (maybe_triangle_collision) {
            out({
                .decoration_id = decoration.id,
                .decoration_index = static_cast<size_t>(scene_collision.id),
                .world_position = maybe_triangle_collision->position,
                .world_distance_from_ray_origin = maybe_triangle_collision->distance,
            });
        }
    });
}

std::vector<SceneCollision> osc::get_all_ray_collisions_with_scene(
    const BVH& scene_bvh,
    SceneCache& cache,
    std::span<const SceneDecoration> decorations,
    const Ray& world_space_ray)
{
    std::vector<SceneCollision> rv;
    for_each_ray_collision_with_scene(scene_bvh, cache, decorations, world_space_ray, [&rv](SceneCollision&& scene_collision)
    {
        rv.push_back(std::move(scene_collision));
    });
    return rv;
}

std::optional<RayCollision> osc::get_closest_world_space_ray_triangle_collision(
    const Mesh& mesh,
    const BVH& triangle_bvh,
    const Transform& transform,
    const Ray& world_space_ray)
{
    if (mesh.topology() != MeshTopology::Triangles) {
        return std::nullopt;
    }

    // map the ray into the mesh's model space, so that we compute a ray-mesh collision
    const Ray modespace_ray = inverse_transform_ray(world_space_ray, transform);

    // then perform a ray-AABB (of triangles) collision
    std::optional<RayCollision> rv;
    triangle_bvh.for_each_ray_aabb_collision(modespace_ray, [&mesh, &transform, &world_space_ray, &modespace_ray, &rv](BVHCollision model_space_bvh_collision)
    {
        // then perform a ray-triangle collision
        if (auto model_space_triangle_collision = find_collision(modespace_ray, mesh.get_triangle_at(model_space_bvh_collision.id))) {
            // map it back into world space and check if it's closer
            const Vector3 world_space_position = transform * model_space_triangle_collision->position;
            const float distance = length(world_space_position - world_space_ray.origin);

            if (not rv or rv->distance > distance) {
                // if it's closer, update the return value
                rv = RayCollision{distance, world_space_position};
            }
        }
    });
    return rv;
}

std::optional<RayCollision> osc::get_closest_world_space_ray_triangle_collision(
    const PolarPerspectiveCamera& camera,
    const Mesh& mesh,
    const BVH& triangle_bvh,
    const Rect& screen_render_rect,
    Vector2 mouse_screen_position)
{
    const Ray world_ray = camera.unproject_topleft_position_to_world_ray(
        mouse_screen_position - screen_render_rect.ypd_top_left(),
        screen_render_rect.dimensions()
    );

    return get_closest_world_space_ray_triangle_collision(
        mesh,
        triangle_bvh,
        identity<Transform>(),
        world_ray
    );
}

SceneRendererParams osc::calc_standard_dark_scene_render_params(
    const PolarPerspectiveCamera& camera,
    AntiAliasingLevel aa_level,
    Vector2 dimensions,
    float device_pixel_ratio)
{
    return SceneRendererParams{
        .dimensions = dimensions,
        .device_pixel_ratio = device_pixel_ratio,
        .anti_aliasing_level = aa_level,
        .draw_mesh_normals = false,
        .draw_floor = false,
        .near_clipping_plane = camera.znear,
        .far_clipping_plane = camera.zfar,
        .view_matrix = camera.view_matrix(),
        .projection_matrix = camera.projection_matrix(aspect_ratio_of(dimensions)),
        .viewer_position = camera.position(),
        .light_direction = recommended_light_direction(camera),
        .background_color = {0.1f, 1.0f},
    };
}

BVH osc::create_triangle_bvh(const Mesh& mesh)
{
    const auto indices = mesh.indices();

    BVH rv;

    if (indices.empty()) {
        return rv;
    }
    if (mesh.topology() != MeshTopology::Triangles) {
        return rv;
    }

    if (indices.is_uint32()) {
        rv.build_from_indexed_triangles(mesh.vertices() , indices.to_uint32_span());
    }
    else {
        rv.build_from_indexed_triangles(mesh.vertices(), indices.to_uint16_span());
    }
    return rv;
}

FrustumPlanes osc::calc_frustum_planes(const Camera& camera, float aspect_ratio)
{
    const Radians fov_y = camera.vertical_field_of_view();
    const auto [z_near, z_far] = camera.clipping_planes();
    const float half_v_size = z_far * tan(fov_y * 0.5f);
    const float half_h_size = half_v_size * aspect_ratio;
    const Vector3 pos = camera.position();
    const Vector3 front = camera.direction();
    const Vector3 up = camera.upwards_direction();
    const Vector3 right = cross(front, up);
    const Vector3 front_mult_near = z_near * front;
    const Vector3 front_mult_far = z_far * front;

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
