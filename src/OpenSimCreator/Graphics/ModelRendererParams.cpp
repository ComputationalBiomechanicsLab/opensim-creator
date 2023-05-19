#include "ModelRendererParams.hpp"

#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>

osc::ModelRendererParams::ModelRendererParams() :
    decorationOptions{},
    renderingOptions{},
    lightColor{osc::SceneRendererParams{}.lightColor},
    backgroundColor{osc::SceneRendererParams{}.backgroundColor},
    floorLocation{osc::SceneRendererParams{}.floorLocation},
    camera{osc::CreateCameraWithRadius(5.0f)}
{
}