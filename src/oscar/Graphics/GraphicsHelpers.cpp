#include "GraphicsHelpers.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Tetrahedron.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/Assertions.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

using namespace osc;

namespace
{
    // describes the direction of each cube face and which direction is "up"
    // from the perspective of looking at that face from the center of the cube
    struct CubemapFaceDetails final {
        Vec3 direction;
        Vec3 up;
    };
    constexpr auto c_CubemapFacesDetails = std::to_array<CubemapFaceDetails>(
    {
        {{ 1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
        {{-1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
        {{ 0.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}},
        {{ 0.0f, -1.0f,  0.0f}, {0.0f,  0.0f, -1.0f}},
        {{ 0.0f,  0.0f,  1.0f}, {0.0f, -1.0f,  0.0f}},
        {{ 0.0f,  0.0f, -1.0f}, {0.0f, -1.0f,  0.0f}},
    });

    Mat4 CalcCubemapViewMatrix(CubemapFaceDetails const& faceDetails, Vec3 const& cubeCenter)
    {
        return look_at(cubeCenter, cubeCenter + faceDetails.direction, faceDetails.up);
    }
}

std::array<Mat4, 6> osc::CalcCubemapViewProjMatrices(
    Mat4 const& projectionMatrix,
    Vec3 cubeCenter)
{
    static_assert(std::size(c_CubemapFacesDetails) == 6);

    std::array<Mat4, 6> rv{};
    for (size_t i = 0; i < 6; ++i)
    {
        rv[i] = projectionMatrix * CalcCubemapViewMatrix(c_CubemapFacesDetails[i], cubeCenter);
    }
    return rv;
}
