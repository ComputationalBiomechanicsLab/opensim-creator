#include "ModelRendererParams.hpp"

#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"
#include "OpenSimCreator/Graphics/OverlayDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

osc::ModelRendererParams::ModelRendererParams() :
    decorationOptions{},
    renderingOptions{},
    lightColor{osc::SceneRendererParams{}.lightColor},
    backgroundColor{osc::SceneRendererParams{}.backgroundColor},
    floorLocation{osc::SceneRendererParams{}.floorLocation},
    camera{osc::CreateCameraWithRadius(5.0f)}
{
}