#include "ModelRendererParams.hpp"

#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"

osc::ModelRendererParams::ModelRendererParams() :
    decorationOptions{},
    renderingOptions{},
    lightColor{osc::SceneRendererParams{}.lightColor},
    backgroundColor{osc::SceneRendererParams{}.backgroundColor},
    floorLocation{osc::SceneRendererParams{}.floorLocation},
    camera{osc::CreateCameraWithRadius(5.0f)}
{
}