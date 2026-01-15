#pragma once

#include <liboscar/maths/ellipsoid.h>
#include <liboscar/maths/plane.h>
#include <liboscar/maths/sphere.h>

namespace osc { class Mesh; }

namespace osc
{
    Sphere FitSphere(const Mesh&);
    Plane FitPlane(const Mesh&);
    Ellipsoid FitEllipsoid(const Mesh&);
}
