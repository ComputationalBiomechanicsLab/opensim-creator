#include "ConeGeometry.h"

#include <oscar/Graphics/Geometries/CylinderGeometry.h>
#include <oscar/Graphics/Mesh.h>

using namespace osc;

osc::ConeGeometry::ConeGeometry(const Params& p) :

    Mesh{CylinderGeometry{{
        .radius_top = 0.0f,
        .radius_bottom = p.radius,
        .height = p.height,
        .num_radial_segments = p.num_radial_segments,
        .num_height_segments = p.num_height_segments,
        .open_ended = p.open_ended,
        .theta_start = p.theta_start,
        .theta_length = p.theta_length,
    }}}
{
    // the implementation of this was initially translated from `three.js`'s
    // `ConeGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/ConeGeometry
}
