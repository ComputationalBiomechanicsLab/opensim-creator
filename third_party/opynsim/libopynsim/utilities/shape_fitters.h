#pragma once

#include <liboscar/maths/ellipsoid.h>
#include <liboscar/maths/plane.h>
#include <liboscar/maths/sphere.h>

namespace osc { class Mesh; }

namespace opyn
{
    osc::Sphere fit_sphere_htbad(const osc::Mesh&);
    osc::Plane fit_plane_htbad(const osc::Mesh&);
    osc::Ellipsoid fit_ellipsoid_htbad(const osc::Mesh&);
}
