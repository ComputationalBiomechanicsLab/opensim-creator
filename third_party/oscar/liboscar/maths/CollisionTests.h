#pragma once

#include <liboscar/maths/RayCollision.h>
#include <liboscar/maths/Vector2.h>

#include <optional>

namespace osc { class Rect; }
namespace osc { struct AABB; }
namespace osc { struct Disc; }
namespace osc { struct FrustumPlanes;}
namespace osc { struct Ray; }
namespace osc { struct Plane; }
namespace osc { struct Sphere; }
namespace osc { struct Triangle; }

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
