#pragma once

#include <OpenSimCreator/Graphics/CustomRenderingOptions.hpp>
#include <OpenSimCreator/Graphics/OverlayDecorationOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp>

#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <string_view>

namespace osc { class AppConfig; }

namespace osc
{
    struct ModelRendererParams final {
        ModelRendererParams();

        OpenSimDecorationOptions decorationOptions;
        OverlayDecorationOptions overlayOptions;
        CustomRenderingOptions renderingOptions;
        Color lightColor;
        Color backgroundColor;
        Vec3 floorLocation;
        PolarPerspectiveCamera camera;
    };

    void UpdModelRendererParamsFrom(
        AppConfig const&,
        std::string_view keyPrefix,
        ModelRendererParams& params
    );

    void SaveModelRendererParamsDifference(
        ModelRendererParams const&,
        ModelRendererParams const&,
        std::string_view keyPrefix,
        AppConfig& config
    );
}
