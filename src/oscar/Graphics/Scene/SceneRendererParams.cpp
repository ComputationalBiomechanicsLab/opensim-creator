#include "SceneRendererParams.h"

osc::SceneRendererParams::SceneRendererParams() :
    dimensions{1, 1},
    antiAliasingLevel{AntiAliasingLevel::none()},
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
    lightColor{DefaultLightColor()},
    ambientStrength{0.01f},
    diffuseStrength{0.55f},
    specularStrength{0.7f},
    specularShininess{6.0f},
    backgroundColor{DefaultBackgroundColor()},
    rimColor{0.95f, 0.35f, 0.0f, 1.0f},
    rimThicknessInPixels{1.0f, 1.0f},
    floorLocation{DefaultFloorLocation()},
    fixupScaleFactor{1.0f}
{}
