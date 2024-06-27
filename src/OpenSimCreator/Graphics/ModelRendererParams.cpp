#include "ModelRendererParams.h"

#include <OpenSimCreator/Graphics/CustomRenderingOptions.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Graphics/OverlayDecorationOptions.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Variant/Variant.h>

#include <string>
#include <string_view>
#include <unordered_map>

using namespace osc;

namespace
{
    std::unordered_map<std::string, Variant> ToValues(
        std::string_view prefix,
        const ModelRendererParams& params)
    {
        std::unordered_map<std::string, Variant> rv;
        std::string subPrefix;
        const auto callback = [&subPrefix, &rv](std::string_view subkey, Variant value)
        {
            rv.insert_or_assign(subPrefix + std::string{subkey}, std::move(value));
        };

        subPrefix = std::string{prefix} + std::string{"decorations/"};
        params.decorationOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"overlays/"};
        params.overlayOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"graphics/"};
        params.renderingOptions.forEachOptionAsAppSettingValue(callback);
        rv.insert_or_assign(std::string{prefix} + "light_color", params.lightColor);
        rv.insert_or_assign(std::string{prefix} + "background_color", params.backgroundColor);
        // TODO: floorLocation

        return rv;
    }

    void UpdFromValues(
        std::string_view prefix,
        const std::unordered_map<std::string, Variant>& values,
        ModelRendererParams& params)
    {
        params.decorationOptions.tryUpdFromValues(std::string{prefix} + "decorations/", values);
        params.overlayOptions.tryUpdFromValues(std::string{prefix} + "overlays/", values);
        params.renderingOptions.tryUpdFromValues(std::string{prefix} + "graphics/", values);
        if (const auto* v = lookup_or_nullptr(values, std::string{prefix} + "light_color")) {
            params.lightColor = v->to<Color>();
        }
        if (const auto* v = lookup_or_nullptr(values,std::string{prefix} + "background_color")) {
            params.backgroundColor = v->to<Color>();
        }
        // TODO: floorLocation
    }
}

osc::ModelRendererParams::ModelRendererParams() :
    lightColor{SceneRendererParams::default_light_color()},
    backgroundColor{SceneRendererParams::default_background_color()},
    floorLocation{SceneRendererParams::default_floor_location()},
    camera{create_camera_with_radius(5.0f)}
{}

void osc::UpdModelRendererParamsFrom(
    const AppSettings& settings,
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

void osc::SaveModelRendererParamsDifference(
    const ModelRendererParams& a,
    const ModelRendererParams& b,
    std::string_view settingsKeyPrefix,
    AppSettings& settings)
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
