#include "SceneRendererParams.hpp"

osc::SceneRendererParams::SceneRendererParams() :
    dimensions{1, 1},
    samples{1},
    drawMeshNormals{false},
    drawRims{true},
    drawShadows{true},
    drawFloor{true},
    nearClippingPlane{0.1f},
    farClippingPlane{100.0f},
    viewMatrix{1.0f},
    projectionMatrix{1.0f},
    viewPos{0.0f, 0.0f, 0.0f},
    lightDirection{-0.34f, -0.25f, 0.05f},
    lightColor{248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f, 1.0f},
    ambientStrength{0.01f},
    diffuseStrength{0.55f},
    specularStrength{0.7f},
    shininess{6.0f},
    backgroundColor{0.89f, 0.89f, 0.89f, 1.0f},
    rimColor{0.95f, 0.35f, 0.0f, 1.0f},
    rimThicknessInPixels{1.0f, 1.0f},
    floorLocation{0.0f, -0.001f, 0.0f},
    fixupScaleFactor{1.0f}
{
}

bool osc::operator==(SceneRendererParams const& a, SceneRendererParams const& b)
{
    return
        a.dimensions == b.dimensions &&
        a.samples == b.samples &&
        a.drawMeshNormals == b.drawMeshNormals &&
        a.drawRims == b.drawRims &&
        a.drawShadows == b.drawShadows &&
        a.drawFloor == b.drawFloor &&
        a.nearClippingPlane == b.nearClippingPlane &&
        a.farClippingPlane == b.farClippingPlane &&
        a.viewMatrix == b.viewMatrix &&
        a.projectionMatrix == b.projectionMatrix &&
        a.viewPos == b.viewPos &&
        a.lightDirection == b.lightDirection &&
        a.lightColor == b.lightColor &&
        a.ambientStrength == b.ambientStrength &&
        a.diffuseStrength == b.diffuseStrength &&
        a.specularStrength == b.specularStrength &&
        a.shininess == b.shininess &&
        a.backgroundColor == b.backgroundColor &&
        a.rimColor == b.rimColor &&
        a.rimThicknessInPixels == b.rimThicknessInPixels &&
        a.floorLocation == b.floorLocation &&
        a.fixupScaleFactor == b.fixupScaleFactor;
}

bool osc::operator!=(SceneRendererParams const& a, SceneRendererParams const& b)
{
    return !(a == b);
}