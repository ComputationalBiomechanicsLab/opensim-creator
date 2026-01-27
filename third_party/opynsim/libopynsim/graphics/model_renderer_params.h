#pragma once

#include <libopynsim/graphics/custom_rendering_options.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/maths/vector3.h>

#include <string_view>

namespace osc { class AppSettings; }

namespace opyn
{
    struct ModelRendererParams final {
        ModelRendererParams();

        OpenSimDecorationOptions decorationOptions;
        OverlayDecorationOptions overlayOptions;
        osc::CustomRenderingOptions renderingOptions;
        osc::Color lightColor;
        osc::Color backgroundColor;
        osc::Vector3 floorLocation;
        osc::PolarPerspectiveCamera camera;
    };

    void UpdModelRendererParamsFrom(
        const osc::AppSettings&,
        std::string_view keyPrefix,
        ModelRendererParams& params
    );

    void SaveModelRendererParamsDifference(
        const ModelRendererParams&,
        const ModelRendererParams&,
        std::string_view settingsKeyPrefix,
        osc::AppSettings&
    );
}
