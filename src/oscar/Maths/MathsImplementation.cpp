#include <oscar/Maths.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cmath>
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <stack>
#include <stdexcept>
#include <utility>

using namespace osc::literals;
using namespace osc;
namespace rgs = std::ranges;

std::ostream& osc::operator<<(std::ostream& out, const AABB& aabb)
{
    return out << "AABB(min = " << aabb.min << ", max = " << aabb.max << ')';
}

std::ostream& osc::operator<<(std::ostream& out, const AnalyticPlane& plane)
{
    return out << "AnalyticPlane(distance = " << plane.distance << ", normal = " << plane.normal << ')';
}

namespace
{
    bool has_nonzero_volume(const Triangle& t)
    {
        return not (t.p0 == t.p1 or t.p0 == t.p2 or t.p1 == t.p2);
    }

    // recursively build the BVH
    void bvh_recursive_build(
        std::vector<BVHNode>& nodes,
        std::vector<BVHPrim>& prims,
        const ptrdiff_t begin,
        const ptrdiff_t n)
    {
        if (n == 1) {
            // recursion bottomed out: create a leaf node
            nodes.push_back(BVHNode::leaf(
                prims.at(begin).bounds(),
                begin
            ));
            return;
        }

        // else: `n >= 2`, so partition the data appropriately and allocate an internal node

        ptrdiff_t midpoint = -1;
        ptrdiff_t internal_node_loc = -1;
        {
            // allocate an appropriate internal node

            // compute bounding box of remaining (children) prims
            const AABB aabb = bounding_aabb_of(
                std::span<const BVHPrim>{prims.begin() + begin, static_cast<size_t>(n)},
                &BVHPrim::bounds
            );

            // compute slicing position along the longest dimension
            const auto longest_dim_index = max_element_index(dimensions_of(aabb));
            const float midpoint_x2 = aabb.min[longest_dim_index] + aabb.max[longest_dim_index];

            // returns `true` if a given primitive is below the midpoint along the dim
            const auto is_below_midpoint = [longest_dim_index, midpoint_x2](const BVHPrim& p)
            {
                const float prim_midpoint_x2 = p.bounds().min[longest_dim_index] + p.bounds().max[longest_dim_index];
                return prim_midpoint_x2 <= midpoint_x2;
            };

            // partition prims into above/below the midpoint
            const ptrdiff_t end = begin + n;
            const auto it = std::partition(prims.begin() + begin, prims.begin() + end, is_below_midpoint);

            midpoint = std::distance(prims.begin(), it);
            if (midpoint == begin or midpoint == end) {
                // edge-case: failed to spacially partition: just naievely partition
                midpoint = begin + n/2;
            }

            internal_node_loc = static_cast<ptrdiff_t>(nodes.size());

            // push the internal node
            nodes.push_back(BVHNode::node(
                aabb,
                0  // the number of left-hand nodes is set later
            ));
        }

        // build left-hand subtree
        bvh_recursive_build(nodes, prims, begin, midpoint-begin);

        // the left-hand build allocated nodes for the left hand side contiguously in memory
        const ptrdiff_t num_lhs_nodes = (static_cast<ptrdiff_t>(nodes.size()) - 1) - internal_node_loc;
        OSC_ASSERT(num_lhs_nodes > 0);
        nodes[internal_node_loc].set_num_lhs_nodes(num_lhs_nodes);

        // build right node
        bvh_recursive_build(nodes, prims, midpoint, (begin + n) - midpoint);
        OSC_ASSERT(internal_node_loc+num_lhs_nodes < static_cast<ptrdiff_t>(nodes.size()));
    }

    // returns `true` if something hit (recursively)
    //
    // populates outparam with all `AABB` hits in depth-first order
    bool bvh_for_each_ray_aabb_collisions_recursive(
        std::span<const BVHNode> nodes,
        std::span<const BVHPrim> prims,
        const Line& ray,
        ptrdiff_t nodeidx,
        const std::function<void(BVHCollision)>& callback)
    {
        const BVHNode& node = nodes[nodeidx];

        // check ray-AABB intersection with the BVH node
        const std::optional<RayCollision> maybe_collision = find_collision(ray, node.bounds());
        if (not maybe_collision) {
            return false;  // no intersection with this node at all
        }

        if (node.is_leaf()) {
            // it's a leaf node, so we've sucessfully found the `AABB` that intersected
            callback(BVHCollision{
                maybe_collision->distance,
                maybe_collision->position,
                prims[node.first_prim_offset()].id(),
            });
            return true;
        }

        // else: we've "hit" an internal node and need to recurse to find the leaf
        const bool lhs_hit = bvh_for_each_ray_aabb_collisions_recursive(nodes, prims, ray, nodeidx+1, callback);
        const bool rhs_hit = bvh_for_each_ray_aabb_collisions_recursive(nodes, prims, ray, nodeidx+node.num_lhs_nodes()+1, callback);
        return lhs_hit or rhs_hit;
    }

    template<std::unsigned_integral TIndex>
    std::optional<BVHCollision> bvh_get_closest_ray_indexed_triangle_collision_recursive(
        std::span<const BVHNode> nodes,
        std::span<const BVHPrim> prims,
        std::span<const Vec3> vertices,
        std::span<const TIndex> indices,
        const Line& ray,
        float& closest,
        ptrdiff_t nodeidx)
    {
        const BVHNode& node = nodes[nodeidx];
        const std::optional<RayCollision> node_collision = find_collision(ray, node.bounds());

        if (not node_collision) {
            return std::nullopt;  // didn't hit this node at all
        }

        if (node_collision->distance > closest) {
            return std::nullopt;  // this AABB can't contain something closer
        }

        if (node.is_leaf()) {
            // leaf node: check ray-triangle intersection

            const BVHPrim& prim = at(prims, node.first_prim_offset());

            const Triangle triangle = {
                at(vertices, at(indices, prim.id())),
                at(vertices, at(indices, prim.id()+1)),
                at(vertices, at(indices, prim.id()+2)),
            };

            const std::optional<RayCollision> triangle_collision = find_collision(ray, triangle);

            if (triangle_collision and triangle_collision->distance < closest) {
                closest = triangle_collision->distance;
                return BVHCollision{triangle_collision->distance, triangle_collision->position, prim.id()};
            }
            else {
                return std::nullopt;  // it didn't collide with the triangle
            }
        }

        // else: `is_node`, so recurse
        const std::optional<BVHCollision> lhs = bvh_get_closest_ray_indexed_triangle_collision_recursive(
            nodes,
            prims,
            vertices,
            indices,
            ray,
            closest,
            nodeidx+1
        );
        const std::optional<BVHCollision> rhs = bvh_get_closest_ray_indexed_triangle_collision_recursive(
            nodes,
            prims,
            vertices,
            indices,
            ray,
            closest,
            nodeidx+node.num_lhs_nodes()+1
        );

        return rhs ? rhs : lhs;
    }

    template<std::unsigned_integral TIndex>
    void bvh_build_from_indexed_triangles(
        std::vector<BVHNode>& nodes,
        std::vector<BVHPrim>& prims,
        std::span<const Vec3> vertices,
        std::span<const TIndex> indices)
    {
        // clear out any old data
        nodes.clear();
        prims.clear();

        // build up the prim list for each triangle
        prims.reserve(indices.size()/3);  // guess: upper limit
        for (size_t i = 0; (i+2) < indices.size(); i += 3) {
            const Triangle triangle{
                at(vertices, indices[i]),
                at(vertices, indices[i+1]),
                at(vertices, indices[i+2]),
            };

            if (has_nonzero_volume(triangle)) {
                prims.emplace_back(static_cast<ptrdiff_t>(i), bounding_aabb_of(triangle));
            }
        }

        nodes.reserve(2 * prims.size());  // guess
        if (not prims.empty()) {
            bvh_recursive_build(nodes, prims, 0, static_cast<ptrdiff_t>(prims.size()));
        }

        prims.shrink_to_fit();
        nodes.shrink_to_fit();
    }

    template<std::unsigned_integral TIndex>
    std::optional<BVHCollision> bvh_get_closest_ray_indexed_triangle_collision(
        std::span<const BVHNode> nodes,
        std::span<const BVHPrim> prims,
        std::span<const Vec3> vertices,
        std::span<const TIndex> indices,
        const Line& ray)
    {
        if (nodes.empty() or prims.empty() or indices.empty()) {
            return std::nullopt;
        }

        float closest = std::numeric_limits<float>::max();
        return bvh_get_closest_ray_indexed_triangle_collision_recursive(nodes, prims, vertices, indices, ray, closest, 0);
    }

    // describes the direction of each cube face and which direction is "up"
    // from the perspective of looking at that face from the center of the cube
    struct CubemapFaceDetails final {
        Vec3 direction;
        Vec3 up;
    };
    constexpr auto c_CubemapFacesDetails = std::to_array<CubemapFaceDetails>({
        {{ 1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
        {{-1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
        {{ 0.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}},
        {{ 0.0f, -1.0f,  0.0f}, {0.0f,  0.0f, -1.0f}},
        {{ 0.0f,  0.0f,  1.0f}, {0.0f, -1.0f,  0.0f}},
        {{ 0.0f,  0.0f, -1.0f}, {0.0f, -1.0f,  0.0f}},
    });

    Mat4 calc_cubemap_view_matrix(const CubemapFaceDetails& face_details, const Vec3& cube_center)
    {
        return look_at(cube_center, cube_center + face_details.direction, face_details.up);
    }
}

void osc::BVH::clear()
{
    nodes_.clear();
    prims_.clear();
}

void osc::BVH::build_from_indexed_triangles(std::span<const Vec3> vertices, std::span<const uint16_t> indices)
{
    bvh_build_from_indexed_triangles<uint16_t>(
        nodes_,
        prims_,
        vertices,
        indices
    );
}

void osc::BVH::build_from_indexed_triangles(std::span<const Vec3> vertices, std::span<const uint32_t> indices)
{
    bvh_build_from_indexed_triangles<uint32_t>(
        nodes_,
        prims_,
        vertices,
        indices
    );
}

std::optional<BVHCollision> osc::BVH::closest_ray_indexed_triangle_collision(
    std::span<const Vec3> vertices,
    std::span<const uint16_t> indices,
    const Line& line) const
{
    return bvh_get_closest_ray_indexed_triangle_collision<uint16_t>(
        nodes_,
        prims_,
        vertices,
        indices,
        line
    );
}

std::optional<BVHCollision> osc::BVH::closest_ray_indexed_triangle_collision(
    std::span<const Vec3> vertices,
    std::span<const uint32_t> indices,
    const Line& line) const
{
    return bvh_get_closest_ray_indexed_triangle_collision<uint32_t>(
        nodes_,
        prims_,
        vertices,
        indices,
        line
    );
}

void osc::BVH::build_from_aabbs(std::span<const AABB> aabbs)
{
    // clear out any old data
    clear();

    // build up prim list for each AABB (just copy the AABB)
    prims_.reserve(aabbs.size());  // guess
    for (ptrdiff_t i = 0; i < ssize(aabbs); ++i) {
        if (not is_point(aabbs[i])) {
            prims_.emplace_back(i, aabbs[i]);
        }
    }

    nodes_.reserve(2 * prims_.size());
    if (not prims_.empty()) {
        bvh_recursive_build(nodes_, prims_, 0, ssize(prims_));
    }

    prims_.shrink_to_fit();
    nodes_.shrink_to_fit();
}

void osc::BVH::for_each_ray_aabb_collision(
    const Line& ray,
    const std::function<void(BVHCollision)>& callback) const
{
    if (nodes_.empty() or prims_.empty()) {
        return;
    }

    bvh_for_each_ray_aabb_collisions_recursive(
        nodes_,
        prims_,
        ray,
        0,
        callback
    );
}

bool osc::BVH::empty() const
{
    return nodes_.empty();
}

size_t osc::BVH::max_depth() const
{
    size_t cur = 0;
    size_t maxdepth = 0;
    std::stack<size_t> stack;

    while (cur < nodes_.size()) {
        if (nodes_[cur].is_leaf()) {
            // leaf node: compute its depth and continue traversal (if applicable)

            maxdepth = max(maxdepth, stack.size() + 1);

            if (stack.empty()) {
                break;  // nowhere to traverse to: exit
            }
            else {
                // traverse up to a parent node and try the right-hand side
                const size_t next = stack.top();
                stack.pop();
                cur = next + nodes_[next].num_lhs_nodes() + 1;
            }
        }
        else {
            // internal node: push into the (right-hand) history stack and then
            //                traverse to the left-hand side

            stack.push(cur);
            cur += 1;
        }
    }

    return maxdepth;
}

std::optional<AABB> osc::BVH::bounds() const
{
    if (nodes_.empty()) {
        return std::nullopt;
    }
    return nodes_.front().bounds();
}

void osc::BVH::for_each_leaf_node(const std::function<void(const BVHNode&)>& callback) const
{
    for (const BVHNode& node : nodes_) {
        if (node.is_leaf()) {
            callback(node);
        }
    }
}

void osc::BVH::for_each_leaf_or_inner_node(const std::function<void(const BVHNode&)>& callback) const
{
    for (const BVHNode& node : nodes_) {
        callback(node);
    }
}

// `CoordinateAxis` implementation

std::optional<CoordinateAxis> osc::CoordinateAxis::try_parse(std::string_view str)
{
    if (str.size() != 1) {
        return std::nullopt;
    }

    switch (str.front()) {
    case 'x':
    case 'X':
        return CoordinateAxis::x();
    case 'y':
    case 'Y':
        return CoordinateAxis::y();
    case 'z':
    case 'Z':
        return CoordinateAxis::z();
    default:
        return std::nullopt;  // invalid character
    }
}

std::ostream& osc::operator<<(std::ostream& out, CoordinateAxis axis)
{
    switch (axis.index()) {
    case 0: return out << 'x';
    case 1: return out << 'y';
    case 2: return out << 'z';
    default: return out;
    }
}

std::optional<CoordinateDirection> osc::CoordinateDirection::try_parse(std::string_view str)
{
    if (str.empty()) {
        return std::nullopt;  // no input
    }

    // try to consume the leading sign character (if there is one)
    const bool negated = str.front() == '-';
    if (negated or str.front() == '+') {
        str = str.substr(1);
    }

    // parse the axis part (x/y/z)
    const auto axis = CoordinateAxis::try_parse(str);
    if (not axis) {
        return std::nullopt;
    }

    return negated ? CoordinateDirection{*axis, Negative{}} : CoordinateDirection{*axis};
}

std::ostream& osc::operator<<(std::ostream& out, CoordinateDirection direction)
{
    if (direction.is_negated()) {
        out << '-';
    }
    return out << direction.axis();
}

std::ostream& osc::operator<<(std::ostream& out, const Disc& disc)
{
    return out << "Disc(origin = " << disc.origin << ", normal = " << disc.normal << ", radius = " << disc.radius << ')';
}


Vec3 osc::EulerPerspectiveCamera::front() const
{
    return normalize(Vec3{
        cos(yaw) * cos(pitch),
        sin(pitch),
        sin(yaw) * cos(pitch),
    });
}

Vec3 osc::EulerPerspectiveCamera::up() const
{
    return Vec3{0.0f, 1.0f, 0.0f};
}

Vec3 osc::EulerPerspectiveCamera::right() const
{
    return normalize(cross(front(), up()));
}

Mat4 osc::EulerPerspectiveCamera::view_matrix() const
{
    return look_at(origin, origin + front(), up());
}

Mat4 osc::EulerPerspectiveCamera::projection_matrix(float aspect_ratio) const
{
    return perspective(vertical_fov, aspect_ratio, znear, zfar);
}


std::ostream& osc::operator<<(std::ostream& out, const Line& line)
{
    return out << "Line(origin = " << line.origin << ", direction = " << line.direction << ')';
}


std::ostream& osc::operator<<(std::ostream& out, const Plane& plane)
{
    return out << "Plane(origin = " << plane.origin << ", normal = " << plane.normal << ')';
}


namespace
{
    Vec3 PolarToCartesian(Vec3 focus, float radius, Radians theta, Radians phi)
    {
        const float x = radius * sin(theta) * cos(phi);
        const float y = radius * sin(phi);
        const float z = radius * cos(theta) * cos(phi);

        return -focus + Vec3{x, y, z};
    }
}

osc::PolarPerspectiveCamera::PolarPerspectiveCamera() :
    radius{1.0f},
    theta{45_deg},
    phi{45_deg},
    focus_point{0.0f, 0.0f, 0.0f},
    vertical_fov{35_deg},
    znear{0.1f},
    zfar{100.0f}
{}

void osc::PolarPerspectiveCamera::reset()
{
    *this = {};
}

void osc::PolarPerspectiveCamera::pan(float aspect_ratio, Vec2 delta)
{
    const auto horizontal_fov = vertial_to_horizontal_fov(vertical_fov, aspect_ratio);

    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    const float x_amount =  delta.x * (2.0f * tan(horizontal_fov / 2.0f) * radius);
    const float y_amount = -delta.y * (2.0f * tan(vertical_fov / 2.0f) * radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    const Vec4 default_panning_axis = {x_amount, y_amount, 0.0f, 1.0f};
    const Mat4 rotation_theta = rotate(identity<Mat4>(), theta, UnitVec3::along_y());
    const UnitVec3 theta_vec = UnitVec3{sin(theta), 0.0f, cos(theta)};
    const UnitVec3 phi_axis = cross(theta_vec, UnitVec3::along_y());
    const Mat4 rotation_phi = rotate(identity<Mat4>(), phi, phi_axis);

    const Vec4 panning_axes = rotation_phi * rotation_theta * default_panning_axis;
    focus_point += Vec3{panning_axes};
}

void osc::PolarPerspectiveCamera::drag(Vec2 delta)
{
    theta += 360_deg * -delta.x;
    phi += 360_deg * delta.y;
}

void osc::PolarPerspectiveCamera::rescale_znear_and_zfar_based_on_radius()
{
    // znear and zfar are only really dicated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    znear = 0.02f * radius;
    zfar = 20.0f * radius;
}

Mat4 osc::PolarPerspectiveCamera::view_matrix() const
{
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's
    // a polar coordinate system that shifts the world based on the camera pan

    const Mat4 theta_rotation = rotate(identity<Mat4>(), -theta, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 theta_vec = normalize(Vec3{sin(theta), 0.0f, cos(theta)});
    const Vec3 phi_axis = cross(theta_vec, Vec3{0.0, 1.0f, 0.0f});
    const Mat4 phi_rotation = rotate(identity<Mat4>(), -phi, phi_axis);
    const Mat4 pan_translation = translate(identity<Mat4>(), focus_point);
    return look_at(
        Vec3(0.0f, 0.0f, radius),
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3{0.0f, 1.0f, 0.0f}) * theta_rotation * phi_rotation * pan_translation;
}

Mat4 osc::PolarPerspectiveCamera::projection_matrix(float aspect_ratio) const
{
    return perspective(vertical_fov, aspect_ratio, znear, zfar);
}

Vec3 osc::PolarPerspectiveCamera::position() const
{
    return PolarToCartesian(focus_point, radius, theta, phi);
}

Vec2 osc::PolarPerspectiveCamera::project_onto_screen_rect(
    const Vec3& worldspace_location,
    const Rect& screen_rect) const
{
    const Vec2 screen_dims = dimensions_of(screen_rect);
    const Mat4 view_proj_mtx = projection_matrix(screen_dims.x/screen_dims.y) * view_matrix();

    Vec4 ndc = view_proj_mtx * Vec4{worldspace_location, 1.0f};
    ndc /= ndc.w;  // perspective divide

    Vec2 ndc2D;
    ndc2D = {ndc.x, -ndc.y};        // [-1, 1], Y points down
    ndc2D += 1.0f;                  // [0, 2]
    ndc2D *= 0.5f;                  // [0, 1]
    ndc2D *= screen_dims;           // [0, w]
    ndc2D += screen_rect.p1;        // [x, x + w]

    return ndc2D;
}

Line osc::PolarPerspectiveCamera::unproject_topleft_pos_to_world_ray(Vec2 pos, Vec2 dimensions) const
{
    return perspective_unproject_topleft_screen_pos_to_world_ray(
        pos / dimensions,
        this->position(),
        view_matrix(),
        projection_matrix(dimensions.x/dimensions.y)
    );
}

PolarPerspectiveCamera osc::create_camera_with_radius(float radius)
{
    PolarPerspectiveCamera rv;
    rv.radius = radius;
    return rv;
}

PolarPerspectiveCamera osc::create_camera_focused_on(const AABB& aabb)
{
    PolarPerspectiveCamera rv;
    auto_focus(rv, aabb);
    return rv;
}

Vec3 osc::recommended_light_direction(const PolarPerspectiveCamera& camera)
{
    // theta should track with the camera, so that the scene is always
    // illuminated from the viewer's perspective (#275)
    //
    // and the offset angle should try to closely match other GUIs, which tend to
    // light scenes right-to-left (almost +1 in Z, but slightly along -X also) - #590
    //
    // but don't offset this too much, because we are using double-sided normals
    // (#318, #168) and, if the camera is too angled relative to the PoV, it's
    // possible to see angled parts of the scene be illuminated from the back (which
    // should be impossible)
    const Radians theta = camera.theta + 22.5_deg;

    // #549: phi shouldn't track with the camera, because changing the "height"/"slope"
    // of the camera with shadow rendering (#10) looks bizzare
    const Radians phi = 45_deg;

    const Vec3 p = PolarToCartesian(camera.focus_point, camera.radius, theta, phi);

    return normalize(-camera.focus_point - p);
}

void osc::focus_along_axis(PolarPerspectiveCamera& camera, size_t axis, bool negate)
{
    if (negate) {
        switch (axis) {
        case 0: focus_along_minus_x(camera); break;
        case 1: focus_along_minus_y(camera); break;
        case 2: focus_along_minus_z(camera); break;
        default: break;
        }
    }
    else {
        switch (axis) {
        case 0: focus_along_x(camera); break;
        case 1: focus_along_y(camera); break;
        case 2: focus_along_z(camera); break;
        default: break;
        }
    }
}

void osc::focus_along_x(PolarPerspectiveCamera& camera)
{
    camera.theta = 90_deg;
    camera.phi = 0_deg;
}

void osc::focus_along_minus_x(PolarPerspectiveCamera& camera)
{
    camera.theta = -90_deg;
    camera.phi = 0_deg;
}

void osc::focus_along_y(PolarPerspectiveCamera& camera)
{
    camera.theta = 0_deg;
    camera.phi = 90_deg;
}

void osc::focus_along_minus_y(PolarPerspectiveCamera& camera)
{
    camera.theta = 0_deg;
    camera.phi = -90_deg;
}

void osc::focus_along_z(PolarPerspectiveCamera& camera)
{
    camera.theta = 0_deg;
    camera.phi = 0_deg;
}

void osc::focus_along_minus_z(PolarPerspectiveCamera& camera)
{
    camera.theta = 180_deg;
    camera.phi = 0_deg;
}

void osc::zoom_in(PolarPerspectiveCamera& camera)
{
    camera.radius *= 0.8f;
}

void osc::zoom_out(PolarPerspectiveCamera& camera)
{
    camera.radius *= 1.2f;
}

void osc::reset(PolarPerspectiveCamera& camera)
{
    camera = {};
    camera.theta = 45_deg;
    camera.phi = 45_deg;
}

void osc::auto_focus(
    PolarPerspectiveCamera& camera,
    const AABB& element_aabb,
    float aspect_ratio)
{
    const Sphere bounding_sphere = bounding_sphere_of(element_aabb);
    const Radians smallest_fov = aspect_ratio > 1.0f ? camera.vertical_fov : vertial_to_horizontal_fov(camera.vertical_fov, aspect_ratio);

    // auto-focus the camera with a minimum radius of 1m
    //
    // this will break autofocusing on very small models (e.g. insect legs) but
    // handles the edge-case of autofocusing an empty model (#552), which is a
    // more common use-case (e.g. for new users and users making human-sized models)
    camera.focus_point = -bounding_sphere.origin;
    camera.radius = max(bounding_sphere.radius / tan(smallest_fov/2.0), 1.0f);
    camera.rescale_znear_and_zfar_based_on_radius();
}


std::ostream& osc::operator<<(std::ostream& out, const Rect& rect)
{
    return out << "Rect(p1 = " << rect.p1 << ", p2 = " << rect.p2 << ")";
}


std::ostream& osc::operator<<(std::ostream& out, const LineSegment& line_segment)
{
    return out << "LineSegment(start = " << line_segment.start << ", end = " << line_segment.end << ')';
}


std::ostream& osc::operator<<(std::ostream& out, const Sphere& sphere)
{
    return out << "Sphere(origin = " << sphere.origin << ", radius = " << sphere.radius << ')';
}


float osc::volume_of(const Tetrahedron& tetrahedron)
{
    // sources:
    //
    // http://forums.cgsociety.org/t/how-to-calculate-center-of-mass-for-triangular-mesh/1309966
    // https://stackoverflow.com/questions/9866452/calculate-volume-of-any-tetrahedron-given-4-points

    const Mat4 mat{
        Vec4{tetrahedron[0], 1.0f},
        Vec4{tetrahedron[1], 1.0f},
        Vec4{tetrahedron[2], 1.0f},
        Vec4{tetrahedron[3], 1.0f},
    };

    return determinant_of(mat) / 6.0f;
}


namespace
{
    struct QuadraticFormulaResult final {
        bool computeable;
        float x0;
        float x1;
    };

    // solve a quadratic formula
    //
    // only real-valued results supported - no complex-plane results
    QuadraticFormulaResult solve_quadratic(float a, float b, float c)
    {
        QuadraticFormulaResult res{};

        // b2 - 4ac
        const float discriminant = b*b - 4.0f*a*c;

        if (discriminant < 0.0f) {
            res.computeable = false;
            return res;
        }

        // q = -1/2 * (b +- sqrt(b2 - 4ac))
        const float q = -0.5f * (b + copysign(sqrt(discriminant), b));

        // you might be wondering why this doesn't just compute a textbook
        // version of the quadratic equation (-b +- sqrt(disc))/2a
        //
        // the reason is because `-b +- sqrt(b2 - 4ac)` can result in catastrophic
        // cancellation if `-b` is close to `sqrt(disc)`
        //
        // so, instead, we use two similar, complementing, quadratics:
        //
        // the textbook one:
        //
        //     x = (-b +- sqrt(disc)) / 2a
        //
        // and the "Muller's method" one:
        //
        //     x = 2c / (-b -+ sqrt(disc))
        //
        // the great thing about these two is that the "+-" part of their
        // equations are complements, so you can have:
        //
        // q = -0.5 * (b + sign(b)*sqrt(disc))
        //
        // which, handily, will only *accumulate* the sum inside those
        // parentheses. If `b` is positive, you end up with a positive
        // number. If `b` is negative, you end up with a negative number. No
        // catastropic cancellation. By multiplying it by "-0.5" you end up
        // with:
        //
        //     -b - sqrt(disc)
        //
        // or, if B was negative:
        //
        //     -b + sqrt(disc)
        //
        // both of which are valid terms of both the quadratic equations above
        //
        // see:
        //
        //     https://math.stackexchange.com/questions/1340267/alternative-quadratic-formula
        //     https://en.wikipedia.org/wiki/Quadratic_equation


        res.computeable = true;
        res.x0 = q/a;  // textbook "complete the square" equation
        res.x1 = c/q;  // Muller's method equation
        return res;
    }

    std::optional<RayCollision> find_collision_analytic(const Sphere& sphere, const Line& line)
    {
        // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

        const Vec3 L = line.origin - sphere.origin;

        // coefficients of the quadratic implicit:
        //
        //     P2 - R2 = 0
        //     (O + tD)2 - R2 = 0
        //     (O + tD - C)2 - R2 = 0
        //
        // where:
        //
        //     P    a point on the surface of the sphere
        //     R    the radius of the sphere
        //     O    origin of line
        //     t    scaling factor for line direction (we want this)
        //     D    direction of line
        //     C    center of sphere
        //
        // if the quadratic has solutions, then there must exist one or two
        // `t`s that are points on the sphere's surface.

        const float a = dot(line.direction, line.direction);  // always == 1.0f if d is normalized
        const float b = 2.0f * dot(line.direction, L);
        const float c = dot(L, L) - dot(sphere.radius, sphere.radius);

        auto [ok, t0, t1] = solve_quadratic(a, b, c);

        if (not ok) {
            return std::nullopt;
        }

        // ensure X0 < X1
        if (t0 > t1) {
            std::swap(t0, t1);
        }

        // ensure it's in front
        if (t0 < 0.0f) {
            t0 = t1;
            if (t0 < 0.0f) {
                return std::nullopt;
            }
        }

        return RayCollision{t0, line.origin + t0*line.direction};
    }
}


Radians osc::vertial_to_horizontal_fov(Radians vertical_fov, float aspect_ratio)
{
    // https://en.wikipedia.org/wiki/Field_of_view_in_video_games#Field_of_view_calculations

    return 2.0f * atan(tan(0.5f * vertical_fov) * aspect_ratio);
}


Mat4 osc::mat4_transform_between_directions(const Vec3& dir1, const Vec3& dir2)
{
    const float cos_theta = dot(dir1, dir2);

    if (cos_theta >= static_cast<float>(1.0f) - epsilon_v<float>) {
        // `a` and `b` point in the same direction: return identity transform
        return identity<Mat4>();
    }

    Radians theta{};
    Vec3 rotation_axis{};
    if (cos_theta < static_cast<float>(-1.0f) + epsilon_v<float>) {
        // `a` and `b` point in opposite directions
        //
        // - there is no "ideal" rotation axis
        // - so we try "guessing" one and hope it's good (then try another if it isn't)

        rotation_axis = cross(Vec3{0.0f, 0.0f, 1.0f}, dir1);
        if (length2(rotation_axis) < epsilon_v<float>) {
            // bad luck: they were parallel - use a different axis
            rotation_axis = cross(Vec3{1.0f, 0.0f, 0.0f}, dir1);
        }

        theta = 180_deg;
        rotation_axis = normalize(rotation_axis);
    }
    else {
        theta = acos(cos_theta);
        rotation_axis = normalize(cross(dir1, dir2));
    }

    return rotate(identity<Mat4>(), theta, rotation_axis);
}

Eulers osc::extract_eulers_xyz(const Quat& quaternion)
{
    return extract_eulers_xyz(mat4_cast(quaternion));
}

Vec2 osc::topleft_relative_pos_to_ndc_point(Vec2 relative_pos)
{
    relative_pos.y = 1.0f - relative_pos.y;
    return 2.0f*relative_pos - 1.0f;
}

Vec2 osc::ndc_point_to_topleft_relative_pos(Vec2 ndc_pos)
{
    ndc_pos = (ndc_pos + 1.0f) * 0.5f;
    ndc_pos.y = 1.0f - ndc_pos.y;
    return ndc_pos;
}

Vec4 osc::topleft_relative_pos_to_ndc_cube(Vec2 relative_pos)
{
    return {topleft_relative_pos_to_ndc_point(relative_pos), -1.0f, 1.0f};
}

Line osc::perspective_unproject_topleft_screen_pos_to_world_ray(
    Vec2 relative_pos,
    Vec3 camera_worldspace_origin,
    const Mat4& camera_view_matrix,
    const Mat4& camera_proj_matrix)
{
    // position of point, as if it were on the front of the 3D NDC cube
    const Vec4 line_origin_ndc = topleft_relative_pos_to_ndc_cube(relative_pos);

    Vec4 line_origin_view = inverse(camera_proj_matrix) * line_origin_ndc;
    line_origin_view /= line_origin_view.w;  // perspective divide

    // location of mouse in worldspace
    const Vec3 line_origin_world = Vec3{inverse(camera_view_matrix) * line_origin_view};

    // direction vector from camera to mouse location (i.e. the projection)
    const Vec3 line_direction_world = normalize(line_origin_world - camera_worldspace_origin);

    return Line{
        .origin = line_origin_world,
        .direction = line_direction_world,
    };
}

Vec2 osc::bottom_left_lh(const Rect& rect)
{
    return Vec2{min(rect.p1.x, rect.p2.x), max(rect.p1.y, rect.p2.y)};
}

Rect osc::bounding_rect_of(const Circle& circle)
{
    const float hypot = sqrt(2.0f * circle.radius * circle.radius);
    return {circle.origin - hypot, circle.origin + hypot};
}

Rect osc::expand_by_absolute_amount(const Rect& rect, float abs_amount)
{
    Rect rv{
        elementwise_min(rect.p1, rect.p2),
        elementwise_max(rect.p1, rect.p2)
    };
    rv.p1.x -= abs_amount;
    rv.p2.x += abs_amount;
    rv.p1.y -= abs_amount;
    rv.p2.y += abs_amount;
    return rv;
}

Rect osc::expand_by_absolute_amount(const Rect& rect, Vec2 abs_amount)
{
    Rect rv{
        elementwise_min(rect.p1, rect.p2),
        elementwise_max(rect.p1, rect.p2)
    };
    rv.p1.x -= abs_amount.x;
    rv.p2.x += abs_amount.x;
    rv.p1.y -= abs_amount.y;
    rv.p2.y += abs_amount.y;
    return rv;
}

Rect osc::clamp(const Rect& r, const Vec2& min, const Vec2& max)
{
    return{
        elementwise_clamp(r.p1, min, max),
        elementwise_clamp(r.p2, min, max),
    };
}

Rect osc::ndc_rect_to_screenspace_viewport_rect(const Rect& ndc_rect, const Rect& viewport)
{
    const Vec2 viewport_dimensions = dimensions_of(viewport);

    // remap [-1, 1] into [0, viewport_dimensions]
    Rect rv{
        0.5f * (ndc_rect.p1 + 1.0f) * viewport_dimensions,
        0.5f * (ndc_rect.p2 + 1.0f) * viewport_dimensions,
    };

    // offset by viewport's top-left
    rv.p1 += viewport.p1;
    rv.p2 += viewport.p1;

    return rv;
}

Sphere osc::bounding_sphere_of(std::span<const Vec3> points)
{
    // edge-case: no points provided
    if (points.empty()) {
        return Sphere{.radius = 0.0f};
    }

    const Vec3 origin = centroid_of(bounding_aabb_of(points));

    float r2 = 0.0f;
    for (const Vec3& point : points) {
        r2 = max(r2, length2(point - origin));
    }

    return {.origin = origin, .radius = sqrt(r2)};
}

Sphere osc::bounding_sphere_of(const AABB& aabb)
{
    return bounding_sphere_of(corner_vertices_of(aabb));
}

AABB osc::bounding_aabb_of(const Sphere& sphere)
{
    AABB rv{};
    rv.min = sphere.origin - sphere.radius;
    rv.max = sphere.origin + sphere.radius;
    return rv;
}

Line osc::transform_line(const Line& line, const Mat4& mat)
{
    Line rv{};
    rv.direction = mat * Vec4{line.direction, 0.0f};
    rv.origin = mat * Vec4{line.origin, 1.0f};
    return rv;
}

Line osc::inverse_transform_line(const Line& line, const Transform& transform)
{
    return Line{
        inverse_transform_point(transform, line.origin),
        inverse_transform_direction(transform, line.direction),
    };
}

Mat4 osc::mat4_transform_between(const Disc& src_disc, const Disc& dest_disc)
{
    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction

    // scale factor
    const float s = dest_disc.radius/src_disc.radius;

    // LERP the axes as follows
    //
    // - 1.0f if parallel with N
    // - s if perpendicular to N
    // - N is a directional vector, so it's `cos(theta)` in each axis already
    // - 1-N is sin(theta) of each axis to the normal
    // - LERP is 1.0f + (s - 1.0f)*V, where V is how perpendiular each axis is

    const Vec3 scalers = 1.0f + ((s - 1.0f) * abs(1.0f - src_disc.normal));
    const Mat4 scaler = scale(identity<Mat4>(), scalers);

    float cos_theta = dot(src_disc.normal, dest_disc.normal);
    Mat4 rotator;
    if (cos_theta > 0.9999f) {
        rotator = identity<Mat4>();
    }
    else {
        const Radians theta = acos(cos_theta);
        const Vec3 axis = cross(src_disc.normal, dest_disc.normal);
        rotator = rotate(identity<Mat4>(), theta, axis);
    }

    const Mat4 translator = translate(identity<Mat4>(), dest_disc.origin-src_disc.origin);

    return translator * rotator * scaler;
}

std::array<Vec3, 8> osc::corner_vertices_of(const AABB& aabb)
{
    Vec3 dims = dimensions_of(aabb);

    std::array<Vec3, 8> rv{};
    rv[0] = aabb.min;
    rv[1] = aabb.max;
    size_t pos = 2;
    for (Vec3::size_type i = 0; i < 3; ++i) {
        Vec3 min = aabb.min;
        min[i] += dims[i];
        Vec3 max = aabb.max;
        max[i] -= dims[i];
        rv[pos++] = min;
        rv[pos++] = max;
    }
    return rv;
}

AABB osc::transform_aabb(const Mat4& mat, const AABB& aabb)
{
    return bounding_aabb_of(corner_vertices_of(aabb), [&](const Vec3& vertex)
    {
        const Vec4 p = mat * Vec4{vertex, 1.0f};
        return Vec3{p / p.w};  // perspective divide
    });
}

AABB osc::transform_aabb(const Transform& transform, const AABB& aabb)
{
    // from real-time collision detection (the book)
    //
    // screenshot: https://twitter.com/Herschel/status/1188613724665335808

    const Mat3 mat = mat3_cast(transform);

    AABB rv = bounding_aabb_of(transform.position);  // add in the translation
    for (Vec3::size_type i = 0; i < 3; ++i) {

        // form extent by summing smaller and larger terms respectively
        for (Vec3::size_type j = 0; j < 3; ++j) {
            const float e = mat[j][i] * aabb.min[j];
            const float f = mat[j][i] * aabb.max[j];

            if (e < f) {
                rv.min[i] += e;
                rv.max[i] += f;
            }
            else {
                rv.min[i] += f;
                rv.max[i] += e;
            }
        }
    }
    return rv;
}

std::optional<Rect> osc::loosely_project_into_ndc(
    const AABB& aabb,
    const Mat4& view_mat,
    const Mat4& proj_mat,
    float znear,
    float zfar)
{
    // create a new AABB in viewspace that bounds the worldspace AABB
    AABB viewspace_aabb = transform_aabb(view_mat, aabb);

    // z-test the viewspace AABB to see if any part of it it falls within the
    // camera's clipping planes
    //
    // care: `znear` and `zfar` are usually defined as positive distances from the
    //       camera but viewspace points along -Z

    if (viewspace_aabb.min.z > -znear and viewspace_aabb.max.z > -znear) {
        return std::nullopt;  // AABB out of NDC bounds
    }
    if (viewspace_aabb.min.z < -zfar and viewspace_aabb.max.z < -zfar) {
        return std::nullopt;  // AABB out of NDC bounds
    }

    // clamp the viewspace AABB to within the camera's clipping planes
    viewspace_aabb.min.z = clamp(viewspace_aabb.min.z, -zfar, -znear);
    viewspace_aabb.max.z = clamp(viewspace_aabb.max.z, -zfar, -znear);

    // transform it into an NDC-aligned NDC-space AABB
    const AABB ndc_aabb = transform_aabb(proj_mat, viewspace_aabb);

    // take the X and Y coordinates of that AABB and ensure they are clamped to within bounds
    Rect rv{Vec2{ndc_aabb.min}, Vec2{ndc_aabb.max}};
    rv.p1 = elementwise_clamp(rv.p1, {-1.0f, -1.0f}, {1.0f, 1.0f});
    rv.p2 = elementwise_clamp(rv.p2, {-1.0f, -1.0f}, {1.0f, 1.0f});

    return rv;
}

Mat4 osc::mat4_transform_between(const LineSegment& a, const LineSegment& b)
{
    const Vec3 a1_to_a2 = a.end - a.start;
    const Vec3 b1_to_b2 = b.end - b.start;

    const float a_length = length(a1_to_a2);
    const float b_length = length(b1_to_b2);

    const Vec3 a_direction = a1_to_a2 / a_length;
    const Vec3 b_direction = b1_to_b2 / b_length;

    const Vec3 a_center = (a.start + a.end)/2.0f;
    const Vec3 b_center = (b.start + b.end)/2.0f;

    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction
    const float s = b_length/a_length;
    const Vec3 scaler = Vec3{1.0f, 1.0f, 1.0f} + (s-1.0f)*a_direction;

    const Mat4 rotate = mat4_transform_between_directions(a_direction, b_direction);
    const Mat4 move = translate(identity<Mat4>(), b_center - a_center);

    return move * rotate * scale(identity<Mat4>(), scaler);
}

Transform osc::transform_between(const LineSegment& a, const LineSegment& b)
{
    const Vec3 a1_to_a2 = a.end - a.start;
    const Vec3 b1_to_b2 = b.end - b.start;

    const float a_length = length(a1_to_a2);
    const float b_length = length(b1_to_b2);

    const Vec3 a_direction = a1_to_a2 / a_length;
    const Vec3 b_direction = b1_to_b2 / b_length;

    const Vec3 a_center = (a.start + a.end)/2.0f;
    const Vec3 b_center = (b.start + b.end)/2.0f;

    // for scale: LERP [0,1] onto [1,l] along original direction
    return Transform{
        .scale = Vec3{1.0f, 1.0f, 1.0f} + ((b_length/a_length - 1.0f)*a_direction),
        .rotation = rotation(a_direction, b_direction),
        .position = b_center - a_center,
    };
}

Transform osc::cylinder_to_line_segment_transform(const LineSegment& line_segment, float radius)
{
    const LineSegment cylinder_line{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Transform t = transform_between(cylinder_line, line_segment);
    t.scale.x = radius;
    t.scale.z = radius;
    return t;
}

Transform osc::y_to_y_cone_to_segment_transform(const LineSegment& line_segment, float radius)
{
    return cylinder_to_line_segment_transform(line_segment, radius);
}

Vec3 osc::transform_point(const Mat4& mat, const Vec3& point)
{
    return Vec3{mat * Vec4{point, 1.0f}};
}

Quat osc::to_worldspace_rotation_quat(const Eulers& eulers)
{
    static_assert(std::is_same_v<Eulers::value_type, Radians>);
    return normalize(Quat{Vec3{eulers.x.count(), eulers.y.count(), eulers.z.count()}});
}

void osc::apply_worldspace_rotation(
    Transform& transform,
    const Eulers& euler_angles,
    const Vec3& rotation_center)
{
    Quat q = to_worldspace_rotation_quat(euler_angles);
    transform.position = q*(transform.position - rotation_center) + rotation_center;
    transform.rotation = normalize(q*transform.rotation);
}

bool osc::is_intersecting(const Rect& rect, const Vec2& point)
{
    const Vec2 relative_pos = point - rect.p1;
    const Vec2 rect_dims = dimensions_of(rect);

    return
        (0.0f <= relative_pos.x and relative_pos.x <= rect_dims.x) and
        (0.0f <= relative_pos.y and relative_pos.y <= rect_dims.y);
}

bool osc::is_intersecting(const FrustumPlanes& frustum, const AABB& aabb)
{
    return not rgs::any_of(frustum, [&aabb](const auto& plane) { return is_in_front_of(plane, aabb); });
}

std::optional<RayCollision> osc::find_collision(const Line& line, const Sphere& sphere)
{
    return find_collision_analytic(sphere, line);
}

std::optional<RayCollision> osc::find_collision(const Line& line, const AABB& aabb)
{
    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection

    float t0 = std::numeric_limits<float>::lowest();
    float t1 = std::numeric_limits<float>::max();
    for (Vec3::size_type i = 0; i < 3; ++i) {
        const float inv_dir = 1.0f / line.direction[i];
        float t_near = (aabb.min[i] - line.origin[i]) * inv_dir;
        float t_far = (aabb.max[i] - line.origin[i]) * inv_dir;
        if (t_near > t_far) {
            std::swap(t_near, t_far);
        }
        t0 = max(t0, t_near);
        t1 = min(t1, t_far);

        if (t0 > t1) {
            return std::nullopt;
        }
    }

    return RayCollision{t0, line.origin + t0*line.direction};
}

std::optional<RayCollision> osc::find_collision(const Line& line, const Plane& plane)
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
    //
    // effectively, this is evaluating:
    //
    //     P, a point on the plane
    //     P0, the plane's origin (distance from world origin)
    //     N, the plane's normal
    //
    // against: dot(P-P0, N)
    //
    // which must equal zero for any point in the plane. Given that, a line can
    // be parameterized as `P = O + tD` where:
    //
    //     P, point along the line
    //     O, origin of line
    //     t, distance along line direction
    //     D, line direction
    //
    // sub the line equation into the plane equation, rearrange for `t` and you
    // can figure out how far a plane is along a line
    //
    // equation: t = dot(P0 - O, n) / dot(D, n)

    const float denominator = dot(plane.normal, line.direction);

    if (abs(denominator) > 1e-6) {
        const float numerator = dot(plane.origin - line.origin, plane.normal);
        const float distance = numerator / denominator;
        return RayCollision{distance, line.origin + distance*line.direction};
    }
    else {
        // the line is *very* parallel to the plane, which could cause
        // some divide-by-zero havok: pretend it didn't intersect
        return std::nullopt;
    }
}

std::optional<RayCollision> osc::find_collision(const Line& line, const Disc& disc)
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    const std::optional<RayCollision> plane_collision =
        find_collision(line, Plane{.origin = disc.origin, .normal = disc.normal});

    if (not plane_collision) {
        return std::nullopt;
    }

    // figure out whether the plane hit is within the disc's radius
    const Vec3 v = plane_collision->position - disc.origin;
    const float d2 = dot(v, v);
    const float r2 = dot(disc.radius, disc.radius);

    if (d2 > r2) {
        return std::nullopt;
    }

    return plane_collision;
}

std::optional<RayCollision> osc::find_collision(const Line& line, const Triangle& triangle)
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    // compute triangle normal
    const Vec3 N = triangle_normal(triangle);

    // compute dot product between normal and ray
    const float NdotR = dot(N, line.direction);

    // if the dot product is small, then the ray is probably very parallel to
    // the triangle (or, perpendicular to the normal) and doesn't intersect
    if (abs(NdotR) < epsilon_v<float>) {
        return std::nullopt;
    }

    // - v[0] is a known point on the plane
    // - N is a normal to the plane
    // - N.v[0] is the projection of v[0] onto N and indicates how long along N to go to hit some
    //   other point on the plane
    const float D = dot(N, triangle.p0);

    // ok, that's one side of the equation
    //
    // - the other side of the equation is that the same is true for *any* point on the plane
    // - so: D = P.N also
    // - where P == O + tR (our line)
    // - expand: D = (O + tR).N
    // - rearrange:
    //
    //     D = O.N + t.R.N
    //     D - O.N = t.R.N
    //     (D - O.N)/(R.N) = t
    //
    // tah-dah: we have the ray distance
    const float t = -(dot(N, line.origin) - D) / NdotR;

    // if triangle plane is behind line then return early
    if (t < 0.0f) {
        return std::nullopt;
    }

    // intersection point on triangle plane, computed from line equation
    const Vec3 P = line.origin + t*line.direction;

    // figure out if that point is inside the triangle's bounds using the
    // "inside-outside" test

    // test each triangle edge: {0, 1}, {1, 2}, {2, 0}
    for (size_t i = 0; i < 3; ++i) {
        const Vec3& start = triangle[i];
        const Vec3& end = triangle[(i+1)%3];

        // corner[n] to corner[n+1]
        const Vec3 e = end - start;

        // corner[n] to P
        const Vec3 c = P - start;

        // cross product of the above indicates whether the vectors are
        // clockwise or anti-clockwise with respect to eachover. It's a
        // right-handed coord system, so anti-clockwise produces a vector
        // that points in same direction as normal
        const Vec3 ax = cross(e, c);

        // if the dot product of that axis with the normal is <0.0f then
        // the point was "outside"
        if (dot(ax, N) < 0.0f) {
            return std::nullopt;
        }
    }

    return RayCollision{t, line.origin + t*line.direction};
}

float osc::ease_out_elastic(float x)
{
    // adopted from: https://easings.net/#easeOutElastic

    constexpr float c4 = 2.0f*pi_v<float> / 3.0f;
    const float normalized = saturate(x);

    return pow(2.0f, -5.0f*normalized) * sin((normalized*10.0f - 0.75f) * c4) + 1.0f;
}

std::array<Mat4, 6> osc::calc_cubemap_view_proj_matrices(
    const Mat4& projection_matrix,
    Vec3 cube_center)
{
    static_assert(std::size(c_CubemapFacesDetails) == 6);

    std::array<Mat4, 6> rv{};
    for (size_t i = 0; i < 6; ++i) {
        rv[i] = projection_matrix * calc_cubemap_view_matrix(c_CubemapFacesDetails[i], cube_center);
    }
    return rv;
}
