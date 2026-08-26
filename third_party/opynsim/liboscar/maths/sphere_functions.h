#pragma once

#include <liboscar/maths/sphere.h>
#include <liboscar/maths/vector.h>

#include <optional>
#include <span>

namespace osc { struct AABB; }

namespace osc
{
    // returns a `Sphere` that loosely bounds the given `Vector3`s
    std::optional<Sphere> bounding_sphere_of(std::span<const Vector3>);

    // returns a `Sphere` that loosely bounds the given `AABB`
    Sphere bounding_sphere_of(const AABB&);
}
