#pragma once

#include <liboscar/Graphics/Geometries/BoxGeometry.h>
#include <liboscar/Graphics/Geometries/CircleGeometry.h>
#include <liboscar/Graphics/Geometries/ConeGeometry.h>
#include <liboscar/Graphics/Geometries/CylinderGeometry.h>
#include <liboscar/Graphics/Geometries/DodecahedronGeometry.h>
#include <liboscar/Graphics/Geometries/IcosahedronGeometry.h>
#include <liboscar/Graphics/Geometries/OctahedronGeometry.h>
#include <liboscar/Graphics/Geometries/RingGeometry.h>
#include <liboscar/Graphics/Geometries/SphereGeometry.h>
#include <liboscar/Graphics/Geometries/TetrahedronGeometry.h>
#include <liboscar/Graphics/Geometries/TorusGeometry.h>
#include <liboscar/Graphics/Geometries/TorusKnotGeometry.h>
#include <liboscar/Utils/Typelist.h>

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
