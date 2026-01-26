#pragma once

#include <liboscar/graphics/geometries/box_geometry.h>
#include <liboscar/graphics/geometries/circle_geometry.h>
#include <liboscar/graphics/geometries/cone_geometry.h>
#include <liboscar/graphics/geometries/cylinder_geometry.h>
#include <liboscar/graphics/geometries/dodecahedron_geometry.h>
#include <liboscar/graphics/geometries/icosahedron_geometry.h>
#include <liboscar/graphics/geometries/octahedron_geometry.h>
#include <liboscar/graphics/geometries/ring_geometry.h>
#include <liboscar/graphics/geometries/sphere_geometry.h>
#include <liboscar/graphics/geometries/tetrahedron_geometry.h>
#include <liboscar/graphics/geometries/torus_geometry.h>
#include <liboscar/graphics/geometries/torus_knot_geometry.h>
#include <liboscar/utilities/typelist.h>

namespace osc
{
    using SolidGeometries = Typelist<
        BoxGeometry,
        CircleGeometry,
        ConeGeometry,
        CylinderGeometry,
        DodecahedronGeometry,
        IcosahedronGeometry,
        OctahedronGeometry,
        RingGeometry,
        SphereGeometry,
        TetrahedronGeometry,
        TorusGeometry,
        TorusKnotGeometry
    >;
}
