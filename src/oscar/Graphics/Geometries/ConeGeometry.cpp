#include "ConeGeometry.h"

#include <oscar/Graphics/Geometries/CylinderGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

using namespace osc;

osc::ConeGeometry::ConeGeometry(
    float radius,
    float height,
    size_t num_radial_segments,
    size_t num_height_segments,
    bool open_ended,
    Radians theta_start,
    Radians theta_length) :

    mesh_{CylinderGeometry{
        0.0f,
        radius,
        height,
        num_radial_segments,
        num_height_segments,
        open_ended,
        theta_start,
        theta_length
    }}
{
    // the implementation of this was initially translated from `three.js`'s
    // `ConeGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/ConeGeometry
}
