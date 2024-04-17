#pragma once

#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Vec2.h>

#include <optional>

namespace osc { struct AABB; }
namespace osc { struct Disc; }
namespace osc { struct FrustumPlanes;}
namespace osc { struct Line; }
namespace osc { struct Plane; }
namespace osc { struct Rect; }
namespace osc { struct Sphere; }
namespace osc { struct Triangle; }

namespace osc
{
    bool is_intersecting(const Rect&, const Vec2&);
    bool is_intersecting(const FrustumPlanes&, const AABB&);
    std::optional<RayCollision> find_collision(const Line&, const Sphere&);
    std::optional<RayCollision> find_collision(const Line&, const AABB&);
    std::optional<RayCollision> find_collision(const Line&, const Plane&);
    std::optional<RayCollision> find_collision(const Line&, const Disc&);
    std::optional<RayCollision> find_collision(const Line&, const Triangle&);
}
