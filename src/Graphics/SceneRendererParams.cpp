#include "SceneRendererParams.hpp"

osc::SceneRendererParams::SceneRendererParams() :
    dimensions{1, 1},
    samples{1},
    wireframeMode{false},
    drawMeshNormals{false},
    drawRims{true},
    drawFloor{true},
    viewMatrix{1.0f},
    projectionMatrix{1.0f},
    viewPos{0.0f, 0.0f, 0.0f},
    lightDirection{-0.34f, -0.25f, 0.05f},
    lightColor{248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f},
    backgroundColor{0.89f, 0.89f, 0.89f, 1.0f},
    rimColor{1.0f, 0.4f, 0.0f, 0.85f},
    floorLocation{0.0f, -0.001f, 0.0f},
    fixupScaleFactor{1.0f}
{
}

osc::SceneRendererParams::SceneRendererParams(SceneRendererParams const&) = default;
osc::SceneRendererParams::SceneRendererParams(SceneRendererParams&&) noexcept = default;
osc::SceneRendererParams& osc::SceneRendererParams::operator=(SceneRendererParams const&) = default;
osc::SceneRendererParams& osc::SceneRendererParams::operator=(SceneRendererParams&&) noexcept = default;
osc::SceneRendererParams::~SceneRendererParams() noexcept = default;

bool osc::operator==(SceneRendererParams const& a, SceneRendererParams const& b)
{
    return
        a.dimensions == b.dimensions &&
        a.samples == b.samples &&
        a.wireframeMode == b.wireframeMode &&
        a.drawMeshNormals == b.drawMeshNormals &&
        a.drawRims == b.drawRims &&
        a.drawFloor == b.drawFloor &&
        a.viewMatrix == b.viewMatrix &&
        a.projectionMatrix == b.projectionMatrix &&
        a.viewPos == b.viewPos &&
        a.lightDirection == b.lightDirection &&
        a.lightColor == b.lightColor &&
        a.backgroundColor == b.backgroundColor &&
        a.rimColor == b.rimColor &&
        a.floorLocation == b.floorLocation &&
        a.fixupScaleFactor == b.fixupScaleFactor;
}

bool osc::operator!=(SceneRendererParams const& a, SceneRendererParams const& b)
{
    return !(a == b);
}