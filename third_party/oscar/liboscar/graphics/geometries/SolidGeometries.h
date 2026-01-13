#pragma once

#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/geometries/CircleGeometry.h>
#include <liboscar/graphics/geometries/ConeGeometry.h>
#include <liboscar/graphics/geometries/CylinderGeometry.h>
#include <liboscar/graphics/geometries/DodecahedronGeometry.h>
#include <liboscar/graphics/geometries/IcosahedronGeometry.h>
#include <liboscar/graphics/geometries/OctahedronGeometry.h>
#include <liboscar/graphics/geometries/RingGeometry.h>
#include <liboscar/graphics/geometries/SphereGeometry.h>
#include <liboscar/graphics/geometries/TetrahedronGeometry.h>
#include <liboscar/graphics/geometries/TorusGeometry.h>
#include <liboscar/graphics/geometries/TorusKnotGeometry.h>
#include <liboscar/utils/Typelist.h>

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
