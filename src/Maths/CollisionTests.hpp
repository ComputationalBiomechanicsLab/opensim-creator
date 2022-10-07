#pragma once

#include "src/Maths/RayCollision.hpp"

#include <glm/vec3.hpp>

namespace osc { struct Line; }
namespace osc { struct Sphere; }
namespace osc { struct AABB; }
namespace osc { struct Plane; }
namespace osc { struct Disc; }

namespace osc
{
    RayCollision GetRayCollisionSphere(Line const&, Sphere const&) noexcept;
    RayCollision GetRayCollisionAABB(Line const&, AABB const&) noexcept;
    RayCollision GetRayCollisionPlane(Line const&, Plane const&) noexcept;
    RayCollision GetRayCollisionDisc(Line const&, Disc const&) noexcept;
    RayCollision GetRayCollisionTriangle(Line const&, glm::vec3 const*) noexcept;
}