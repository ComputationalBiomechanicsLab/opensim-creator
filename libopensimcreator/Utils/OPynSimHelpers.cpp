#include "OPynSimHelpers.h"

#include <libopynsim/Utils/TPS3D.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/utils/Perf.h>

#include <new>

using namespace osc;

Mesh osc::TPSWarpMesh(opyn::TPSCoefficients3D<float>& coefs, const Mesh& mesh, float blendingFactor)
{
    OSC_PERF("TPSWarpMesh");

    Mesh rv = mesh;  // make a local copy of the input mesh

    // copy out the vertices

    // parallelize function evaluation, because the mesh may contain *a lot* of
    // vertices and the TPS equation may contain *a lot* of coefficients
    auto vertices = rv.vertices();
    static_assert(alignof(decltype(vertices.front())) == alignof(SimTK::fVec3));
    static_assert(sizeof(decltype(vertices.front())) == sizeof(SimTK::fVec3));
    auto* punned = std::launder(reinterpret_cast<SimTK::fVec3*>(vertices.data()));
    TPSWarpPointsInPlace(coefs, {punned, vertices.size()}, blendingFactor);
    rv.set_vertices(vertices);

    return rv;
}
