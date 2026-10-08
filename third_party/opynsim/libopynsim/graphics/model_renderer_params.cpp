#include "model_renderer_params.h"

#include <libopynsim/graphics/custom_rendering_options.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/maths/angle.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/variant/variant.h>

#include <string>
#include <string_view>
#include <unordered_map>

using namespace opyn;
using namespace osc::literals;

namespace
{
    std::unordered_map<std::string, osc::Variant> to_values(
        std::string_view prefix,
        const ModelRendererParams& params)
    {
        std::unordered_map<std::string, osc::Variant> rv;
        std::string sub_prefix;
        const auto callback = [&sub_prefix, &rv](std::string_view subkey, osc::Variant value)
        {
            rv.insert_or_assign(sub_prefix + std::string{subkey}, std::move(value));
        };

        sub_prefix = std::string{prefix} + std::string{"decorations/"};
        params.decoration_options.for_each_option_as_app_setting_value(callback);
        sub_prefix = std::string{prefix} + std::string{"overlays/"};
        params.overlay_options.for_each_option_as_app_setting_value(callback);
        sub_prefix = std::string{prefix} + std::string{"graphics/"};
        params.rendering_options.for_each_option_as_app_setting_value(callback);
        rv.insert_or_assign(std::string{prefix} + "light_color", osc::Variant{params.light_color});
        rv.insert_or_assign(std::string{prefix} + "background_color", osc::Variant{params.background_color});
        // TODO: floorLocation

        return rv;
    }

    void upd_from_values(
        std::string_view prefix,
        const std::unordered_map<std::string, osc::Variant>& values,
        ModelRendererParams& params)
    {
        params.decoration_options.try_upd_from_values(std::string{prefix} + "decorations/", values);
        params.overlay_options.try_upd_from_values(std::string{prefix} + "overlays/", values);
        params.rendering_options.try_upd_from_values(std::string{prefix} + "graphics/", values);
        if (const auto* v = lookup_or_nullptr(values, std::string{prefix} + "light_color")) {
            params.light_color = to<osc::Color>(*v);
        }
        if (const auto* v = lookup_or_nullptr(values,std::string{prefix} + "background_color")) {
            params.background_color = to<osc::Color>(*v);
        }
        // TODO: floorLocation
    }
}

opyn::ModelRendererParams::ModelRendererParams() :
    light_color{osc::SceneRendererParams::default_light_color()},
    background_color{osc::SceneRendererParams::default_background_color()},
    floor_location{osc::SceneRendererParams::default_floor_position()}
{
    camera.set_vertical_field_of_view(35_deg);
    auto controller = osc::OrbitCameraController{.radius = 5.0f};
    controller.update_camera(camera);
}

void opyn::upd_model_renderer_params_from(
    const osc::AppSettings& settings,
    std::string_view key_prefix,
    ModelRendererParams& params)
{
    auto values = to_values(key_prefix, params);
    for (auto& [k, v] : values) {
        if (auto setting_value = settings.find_value(k)) {
            v = *setting_value;
        }
    }
    upd_from_values(key_prefix, values, params);
}

void opyn::save_model_renderer_params_difference(
    const ModelRendererParams& a,
    const ModelRendererParams& b,
    std::string_view settings_key_prefix,
    osc::AppSettings& settings)
{
    const auto a_vals = to_values(settings_key_prefix, a);
    const auto b_vals = to_values(settings_key_prefix, b);

    for (const auto& [aK, aV] : a_vals) {
        if (const auto* b_v = lookup_or_nullptr(b_vals, aK)) {
            if (*b_v != aV) {
                settings.set_value(aK, *b_v);
            }
        }
    }
}
