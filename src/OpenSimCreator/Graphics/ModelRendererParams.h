#pragma once

#include <OpenSimCreator/Graphics/CustomRenderingOptions.h>
#include <OpenSimCreator/Graphics/OverlayDecorationOptions.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Vec3.h>

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
        const AppConfig&,
        std::string_view keyPrefix,
        ModelRendererParams& params
    );

    void SaveModelRendererParamsDifference(
        const ModelRendererParams&,
        const ModelRendererParams&,
        std::string_view keyPrefix,
        AppConfig& config
    );
}
