#pragma once

#include <libopynsim/Utils/TPS3D.h>
#include <liboscar/Graphics/Mesh.h>

namespace osc
{
    Mesh TPSWarpMesh(opyn::TPSCoefficients3D<float>&, const Mesh&, float blendingFactor);
}
