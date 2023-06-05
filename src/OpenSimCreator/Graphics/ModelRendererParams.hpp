#pragma once

#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"
#include "OpenSimCreator/Graphics/OverlayDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>

#include <glm/vec3.hpp>

namespace osc
{
    struct ModelRendererParams final {
        ModelRendererParams();

        OpenSimDecorationOptions decorationOptions;
        OverlayDecorationOptions overlayOptions;
        CustomRenderingOptions renderingOptions;
        Color lightColor;
        Color backgroundColor;
        glm::vec3 floorLocation;
        PolarPerspectiveCamera camera;
    };
}