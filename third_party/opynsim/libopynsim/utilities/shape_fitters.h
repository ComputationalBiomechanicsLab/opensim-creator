#pragma once

#include <liboscar/maths/ellipsoid.h>
#include <liboscar/maths/plane.h>
#include <liboscar/maths/sphere.h>

namespace osc { class Mesh; }

namespace opyn
{
    osc::Sphere FitSphere(const osc::Mesh&);
    osc::Plane FitPlane(const osc::Mesh&);
    osc::Ellipsoid FitEllipsoid(const osc::Mesh&);
}
