#pragma once

#include <libopensimcreator/Graphics/CustomRenderingOptions.h>
#include <libopensimcreator/Graphics/OpenSimDecorationOptions.h>
#include <libopensimcreator/Graphics/OverlayDecorationOptions.h>

#include <liboscar/graphics/color.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/maths/vector3.h>

#include <string_view>

namespace osc { class AppSettings; }

namespace osc
{
    struct ModelRendererParams final {
        ModelRendererParams();

        OpenSimDecorationOptions decorationOptions;
        OverlayDecorationOptions overlayOptions;
        CustomRenderingOptions renderingOptions;
        Color lightColor;
        Color backgroundColor;
        Vector3 floorLocation;
        PolarPerspectiveCamera camera;
    };

    void UpdModelRendererParamsFrom(
        const AppSettings&,
        std::string_view keyPrefix,
        ModelRendererParams& params
    );

    void SaveModelRendererParamsDifference(
        const ModelRendererParams&,
        const ModelRendererParams&,
        std::string_view settingsKeyPrefix,
        AppSettings&
    );
}
