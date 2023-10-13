#pragma once

#include <oscar/Maths/Ellipsoid.hpp>
#include <oscar/Maths/Plane.hpp>
#include <oscar/Maths/Sphere.hpp>

namespace osc { class Mesh; }

namespace osc
{
    Sphere FitSphere(Mesh const&);
    Plane FitPlane(Mesh const&);
    Ellipsoid FitEllipsoid(Mesh const&);
}
