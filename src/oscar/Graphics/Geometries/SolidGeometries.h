#pragma once

#include <oscar/Graphics/Geometries/BoxGeometry.h>
#include <oscar/Graphics/Geometries/CircleGeometry.h>
#include <oscar/Graphics/Geometries/ConeGeometry.h>
#include <oscar/Graphics/Geometries/CylinderGeometry.h>
#include <oscar/Graphics/Geometries/DodecahedronGeometry.h>
#include <oscar/Graphics/Geometries/IcosahedronGeometry.h>
#include <oscar/Graphics/Geometries/OctahedronGeometry.h>
#include <oscar/Graphics/Geometries/RingGeometry.h>
#include <oscar/Graphics/Geometries/SphereGeometry.h>
#include <oscar/Graphics/Geometries/TetrahedronGeometry.h>
#include <oscar/Graphics/Geometries/TorusGeometry.h>
#include <oscar/Graphics/Geometries/TorusKnotGeometry.h>
#include <oscar/Utils/Typelist.h>

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
