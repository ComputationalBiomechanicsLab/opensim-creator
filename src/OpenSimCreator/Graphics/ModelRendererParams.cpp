#include "ModelRendererParams.hpp"

#include <OpenSimCreator/Graphics/CustomRenderingOptions.hpp>
#include <OpenSimCreator/Graphics/OverlayDecorationOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>

osc::ModelRendererParams::ModelRendererParams() :
    lightColor{SceneRendererParams::DefaultLightColor()},
    backgroundColor{SceneRendererParams::DefaultBackgroundColor()},
    floorLocation{SceneRendererParams::DefaultFloorLocation()},
    camera{CreateCameraWithRadius(5.0f)}
{
}
