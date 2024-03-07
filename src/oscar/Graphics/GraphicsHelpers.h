#pragma once

#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec3.h>

#include <array>

namespace osc
{
    // returns arrays that transforms cube faces from worldspace to projection
    // space such that the observer is looking at each face of the cube from
    // the center of the cube
    std::array<Mat4, 6> CalcCubemapViewProjMatrices(
        Mat4 const& projectionMatrix,
        Vec3 cubeCenter
    );
}
