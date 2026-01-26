#pragma once

#include <libopynsim/graphics/custom_rendering_options.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/overlay_decoration_options.h>
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
        opyn::OverlayDecorationOptions overlayOptions;
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
