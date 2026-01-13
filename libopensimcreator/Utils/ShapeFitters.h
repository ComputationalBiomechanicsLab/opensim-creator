#pragma once

#include <liboscar/maths/Ellipsoid.h>
#include <liboscar/maths/Plane.h>
#include <liboscar/maths/Sphere.h>

namespace osc { class Mesh; }

namespace osc
{
    Sphere FitSphere(const Mesh&);
    Plane FitPlane(const Mesh&);
    Ellipsoid FitEllipsoid(const Mesh&);
}
