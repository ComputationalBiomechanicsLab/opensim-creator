#pragma once

#include <oscar/Maths/RayCollision.hpp>
#include <oscar/Maths/Vec2.hpp>

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
    bool IsPointInRect(Rect const&, Vec2 const&);
    std::optional<RayCollision> GetRayCollisionSphere(Line const&, Sphere const&);
    std::optional<RayCollision> GetRayCollisionAABB(Line const&, AABB const&);
    std::optional<RayCollision> GetRayCollisionPlane(Line const&, Plane const&);
    std::optional<RayCollision> GetRayCollisionDisc(Line const&, Disc const&);
    std::optional<RayCollision> GetRayCollisionTriangle(Line const&, Triangle const&);
}
