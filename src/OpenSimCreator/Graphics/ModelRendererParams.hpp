#pragma once

#include "OpenSimCreator/Graphics/CustomDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>

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