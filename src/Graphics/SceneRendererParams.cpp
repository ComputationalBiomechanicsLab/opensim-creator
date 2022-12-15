#include "SceneRendererParams.hpp"

#include <cstring>

osc::SceneRendererParams::SceneRendererParams() :
    dimensions{1, 1},
    samples{1},
    drawMeshNormals{false},
    drawRims{true},
    drawShadows{false},
    drawFloor{true},
    nearClippingPlane{0.1f},
    farClippingPlane{100.0f},
    viewMatrix{1.0f},
    projectionMatrix{1.0f},
    viewPos{0.0f, 0.0f, 0.0f},
    lightDirection{-0.34f, -0.25f, 0.05f},
    lightColor{248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f},
    ambientStrength{0.15f},
    diffuseStrength{0.85f},
    specularStrength{0.4f},
    shininess{8.0f},
    backgroundColor{0.89f, 0.89f, 0.89f, 1.0f},
    rimColor{1.0f, 0.4f, 0.0f, 1.0f},
    rimThicknessInPixels{1.0f, 1.0f},
    floorLocation{0.0f, -0.001f, 0.0f},
    fixupScaleFactor{1.0f}
{
}

bool osc::operator==(SceneRendererParams const& a, SceneRendererParams const& b)
{
    return std::memcmp(&a, &b, sizeof(SceneRendererParams)) == 0;
}

bool osc::operator!=(SceneRendererParams const& a, SceneRendererParams const& b)
{
    return !(a == b);
}