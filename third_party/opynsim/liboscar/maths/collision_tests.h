#pragma once

#include <liboscar/maths/ray_collision.h>
#include <liboscar/maths/vector.h>

#include <optional>

namespace osc
{
    class Rect;
    struct AABB;
    struct Disc;
    struct FrustumPlanes;
    struct Plane;
    struct Ray;
    struct Sphere;
    struct Triangle;
}

namespace osc
{
    bool is_intersecting(const Rect&, const Vector2&);
    bool is_intersecting(const FrustumPlanes&, const AABB&);
    std::optional<RayCollision> find_collision(const Ray&, const Sphere&);
    std::optional<RayCollision> find_collision(const Ray&, const AABB&);
    std::optional<RayCollision> find_collision(const Ray&, const Plane&);
    std::optional<RayCollision> find_collision(const Ray&, const Disc&);
    std::optional<RayCollision> find_collision(const Ray&, const Triangle&);
}
