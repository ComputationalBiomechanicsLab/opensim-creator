#pragma once

#include <libopynsim/graphics/custom_rendering_options.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/vector.h>

#include <string_view>

namespace osc { class AppSettings; }

namespace opyn
{
    struct ModelRendererParams final {
        ModelRendererParams();

        OpenSimDecorationOptions decoration_options;
        OverlayDecorationOptions overlay_options;
        CustomRenderingOptions rendering_options;
        osc::Color light_color;
        osc::Color background_color;
        osc::Vector3 floor_location;
        osc::Camera camera;
    };

    void upd_model_renderer_params_from(
        const osc::AppSettings&,
        std::string_view keyPrefix,
        ModelRendererParams& params
    );

    void save_model_renderer_params_difference(
        const ModelRendererParams&,
        const ModelRendererParams&,
        std::string_view settingsKeyPrefix,
        osc::AppSettings&
    );
}
