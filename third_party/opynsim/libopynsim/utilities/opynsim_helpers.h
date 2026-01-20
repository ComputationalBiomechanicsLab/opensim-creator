#pragma once

#include <libopynsim/utilities/tps3d.h>
#include <liboscar/graphics/mesh.h>

namespace osc
{
    Mesh TPSWarpMesh(opyn::TPSCoefficients3D<float>&, const Mesh&, float blendingFactor);
}
