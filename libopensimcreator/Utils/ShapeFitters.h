#pragma once

#include <liboscar/Maths/Ellipsoid.h>
#include <liboscar/Maths/Plane.h>
#include <liboscar/Maths/Sphere.h>

namespace osc { class Mesh; }

namespace osc
{
    Sphere FitSphere(const Mesh&);
    Plane FitPlane(const Mesh&);
    Ellipsoid FitEllipsoid(const Mesh&);
}
