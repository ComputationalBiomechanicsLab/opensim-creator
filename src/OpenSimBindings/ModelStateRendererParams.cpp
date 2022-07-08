#include "ModelStateRendererParams.hpp"

osc::ModelStateRendererParams::ModelStateRendererParams() :
    Dimensions{1, 1},
    Samples{1},
    WireframeMode{false},
    DrawMeshNormals{false},
    DrawRims{true},
    DrawFloor{true},
    ViewMatrix{1.0f},
    ProjectionMatrix{1.0f},
    ViewPos{0.0f, 0.0f, 0.0f},
    LightDirection{-0.34f, -0.25f, 0.05f},
    BackgroundColor{0.89f, 0.89f, 0.89f, 1.0f},
    RimColor{1.0f, 0.4f, 0.0f, 0.85f},
    FloorLocation{0.0f, -0.001f, 0.0f},
    FixupScaleFactor{1.0f}
{
}

bool osc::operator==(ModelStateRendererParams const& a, ModelStateRendererParams const& b)
{
    return
        a.Dimensions == b.Dimensions &&
        a.Samples == b.Samples &&
        a.WireframeMode == b.WireframeMode &&
        a.DrawMeshNormals == b.DrawMeshNormals &&
        a.DrawRims == b.DrawRims &&
        a.DrawFloor == b.DrawFloor &&
        a.ViewMatrix == b.ViewMatrix &&
        a.ProjectionMatrix == b.ProjectionMatrix &&
        a.ViewPos == b.ViewPos &&
        a.LightDirection == b.LightDirection &&
        a.LightColor == b.LightColor &&
        a.BackgroundColor == b.BackgroundColor &&
        a.RimColor == b.RimColor &&
        a.FloorLocation == b.FloorLocation &&
        a.FixupScaleFactor == b.FixupScaleFactor;
}
