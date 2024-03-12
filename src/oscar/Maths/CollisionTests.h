#pragma once

#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Vec2.h>

#include <optional>

namespace osc { struct AABB; }
namespace osc { struct Disc; }
namespace osc { struct Line; }
namespace osc { struct Plane; }
namespace osc { struct Rect; }
namespace osc { struct Sphere; }
namespace osc { struct Triangle; }

namespace osc
{
    bool is_intersecting(Rect const&, Vec2 const&);
    std::optional<RayCollision> find_collision(Line const&, Sphere const&);
    std::optional<RayCollision> find_collision(Line const&, AABB const&);
    std::optional<RayCollision> find_collision(Line const&, Plane const&);
    std::optional<RayCollision> find_collision(Line const&, Disc const&);
    std::optional<RayCollision> find_collision(Line const&, Triangle const&);
}
