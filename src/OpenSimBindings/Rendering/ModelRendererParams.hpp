#pragma once

#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/Rendering/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Rendering/CustomRenderingOptions.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace osc
{
    struct ModelRendererParams final {
        CustomDecorationOptions decorationOptions;
        CustomRenderingOptions renderingOptions;
        glm::vec3 lightColor = osc::SceneRendererParams{}.lightColor;
        glm::vec4 backgroundColor = osc::SceneRendererParams{}.backgroundColor;
        glm::vec3 floorLocation = osc::SceneRendererParams{}.floorLocation;
        PolarPerspectiveCamera camera = osc::CreateCameraWithRadius(5.0f);
    };
}