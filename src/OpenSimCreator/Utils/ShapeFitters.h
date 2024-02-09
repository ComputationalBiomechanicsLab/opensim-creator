#pragma once

#include <oscar/Maths/Ellipsoid.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/Sphere.h>

namespace osc { class Mesh; }

namespace osc
{
    Sphere FitSphere(Mesh const&);
    Plane FitPlane(Mesh const&);
    Ellipsoid FitEllipsoid(Mesh const&);
}
