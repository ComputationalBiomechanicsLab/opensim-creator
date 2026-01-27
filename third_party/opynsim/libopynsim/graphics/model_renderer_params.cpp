#include "model_renderer_params.h"

#include <libopynsim/graphics/custom_rendering_options.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/variant/variant.h>

#include <string>
#include <string_view>
#include <unordered_map>

using namespace opyn;

namespace
{
    std::unordered_map<std::string, osc::Variant> ToValues(
        std::string_view prefix,
        const ModelRendererParams& params)
    {
        std::unordered_map<std::string, osc::Variant> rv;
        std::string subPrefix;
        const auto callback = [&subPrefix, &rv](std::string_view subkey, osc::Variant value)
        {
            rv.insert_or_assign(subPrefix + std::string{subkey}, std::move(value));
        };

        subPrefix = std::string{prefix} + std::string{"decorations/"};
        params.decorationOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"overlays/"};
        params.overlayOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"graphics/"};
        params.renderingOptions.forEachOptionAsAppSettingValue(callback);
        rv.insert_or_assign(std::string{prefix} + "light_color", osc::Variant{params.lightColor});
        rv.insert_or_assign(std::string{prefix} + "background_color", osc::Variant{params.backgroundColor});
        // TODO: floorLocation

        return rv;
    }

    void UpdFromValues(
        std::string_view prefix,
        const std::unordered_map<std::string, osc::Variant>& values,
        ModelRendererParams& params)
    {
        params.decorationOptions.tryUpdFromValues(std::string{prefix} + "decorations/", values);
        params.overlayOptions.tryUpdFromValues(std::string{prefix} + "overlays/", values);
        params.renderingOptions.tryUpdFromValues(std::string{prefix} + "graphics/", values);
        if (const auto* v = lookup_or_nullptr(values, std::string{prefix} + "light_color")) {
            params.lightColor = to<osc::Color>(*v);
        }
        if (const auto* v = lookup_or_nullptr(values,std::string{prefix} + "background_color")) {
            params.backgroundColor = to<osc::Color>(*v);
        }
        // TODO: floorLocation
    }
}

opyn::ModelRendererParams::ModelRendererParams() :
    lightColor{osc::SceneRendererParams::default_light_color()},
    backgroundColor{osc::SceneRendererParams::default_background_color()},
    floorLocation{osc::SceneRendererParams::default_floor_position()},
    camera{osc::create_camera_with_radius(5.0f)}
{}

void opyn::UpdModelRendererParamsFrom(
    const osc::AppSettings& settings,
    std::string_view keyPrefix,
    ModelRendererParams& params)
{
    auto values = ToValues(keyPrefix, params);
    for (auto& [k, v] : values) {
        if (auto settingValue = settings.find_value(k)) {
            v = *settingValue;
        }
    }
    UpdFromValues(keyPrefix, values, params);
}

void opyn::SaveModelRendererParamsDifference(
    const ModelRendererParams& a,
    const ModelRendererParams& b,
    std::string_view settingsKeyPrefix,
    osc::AppSettings& settings)
{
    const auto aVals = ToValues(settingsKeyPrefix, a);
    const auto bVals = ToValues(settingsKeyPrefix, b);

    for (const auto& [aK, aV] : aVals) {
        if (const auto* bV = lookup_or_nullptr(bVals, aK)) {
            if (*bV != aV) {
                settings.set_value(aK, *bV);
            }
        }
    }
}
