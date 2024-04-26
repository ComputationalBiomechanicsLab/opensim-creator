#include "ModelRendererParams.h"

#include <OpenSimCreator/Graphics/CustomRenderingOptions.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Graphics/OverlayDecorationOptions.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/AppSettingValue.h>
#include <oscar/Utils/Algorithms.h>

#include <string>
#include <string_view>
#include <unordered_map>

using namespace osc;

namespace
{
    std::unordered_map<std::string, AppSettingValue> ToValues(
        std::string_view prefix,
        ModelRendererParams const& params)
    {
        std::unordered_map<std::string, AppSettingValue> rv;
        std::string subPrefix;
        auto const callback = [&subPrefix, &rv](std::string_view subkey, AppSettingValue value)
        {
            rv.insert_or_assign(subPrefix + std::string{subkey}, std::move(value));
        };

        subPrefix = std::string{prefix} + std::string{"decorations/"};
        params.decorationOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"overlays/"};
        params.overlayOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"graphics/"};
        params.renderingOptions.forEachOptionAsAppSettingValue(callback);
        rv.insert_or_assign(std::string{prefix} + "light_color", AppSettingValue{params.lightColor});
        rv.insert_or_assign(std::string{prefix} + "background_color", AppSettingValue{params.backgroundColor});
        // TODO: floorLocation

        return rv;
    }

    void UpdFromValues(
        std::string_view prefix,
        std::unordered_map<std::string, AppSettingValue> const& values,
        ModelRendererParams& params)
    {
        params.decorationOptions.tryUpdFromValues(std::string{prefix} + "decorations/", values);
        params.overlayOptions.tryUpdFromValues(std::string{prefix} + "overlays/", values);
        params.renderingOptions.tryUpdFromValues(std::string{prefix} + "graphics/", values);
        if (auto const* v = find_or_nullptr(values, std::string{prefix} + "light_color")) {
            params.lightColor = v->to_color();
        }
        if (auto const* v = find_or_nullptr(values,std::string{prefix} + "background_color")) {
            params.backgroundColor = v->to_color();
        }
        // TODO: floorLocation
    }
}

osc::ModelRendererParams::ModelRendererParams() :
    lightColor{SceneRendererParams::default_light_color()},
    backgroundColor{SceneRendererParams::default_background_color()},
    floorLocation{SceneRendererParams::default_floor_location()},
    camera{CreateCameraWithRadius(5.0f)}
{
}

void osc::UpdModelRendererParamsFrom(
    AppConfig const& config,
    std::string_view keyPrefix,
    ModelRendererParams& params)
{
    auto values = ToValues(keyPrefix, params);
    for (auto& [k, v] : values)
    {
        if (auto configV = config.find_value(k))
        {
            v = *configV;
        }
    }
    UpdFromValues(keyPrefix, values, params);
}

void osc::SaveModelRendererParamsDifference(
    ModelRendererParams const& a,
    ModelRendererParams const& b,
    std::string_view keyPrefix,
    AppConfig& config)
{
    auto const aVals = ToValues(keyPrefix, a);
    auto const bVals = ToValues(keyPrefix, b);

    for (auto const& [aK, aV] : aVals) {
        if (auto const* bV = find_or_nullptr(bVals, aK)) {
            if (*bV != aV) {
                config.set_value(aK, *bV);
            }
        }
    }
}
