#pragma once

#include <oscar/Maths/RayCollision.hpp>

#include <glm/vec2.hpp>

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
    bool IsPointInRect(Rect const&, glm::vec2 const&) noexcept;
    std::optional<RayCollision> GetRayCollisionSphere(Line const&, Sphere const&) noexcept;
    std::optional<RayCollision> GetRayCollisionAABB(Line const&, AABB const&) noexcept;
    std::optional<RayCollision> GetRayCollisionPlane(Line const&, Plane const&) noexcept;
    std::optional<RayCollision> GetRayCollisionDisc(Line const&, Disc const&) noexcept;
    std::optional<RayCollision> GetRayCollisionTriangle(Line const&, Triangle const&) noexcept;
}
