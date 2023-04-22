#pragma once

#include "src/Graphics/Color.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/Graphics/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Graphics/CustomRenderingOptions.hpp"

#include <glm/vec3.hpp>

namespace osc
{
    struct ModelRendererParams final {
        ModelRendererParams();

        CustomDecorationOptions decorationOptions;
        CustomRenderingOptions renderingOptions;
        Color lightColor;
        Color backgroundColor;
        glm::vec3 floorLocation;
        PolarPerspectiveCamera camera;
    };
}