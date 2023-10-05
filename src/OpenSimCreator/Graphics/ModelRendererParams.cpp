#include "ModelRendererParams.hpp"

#include <OpenSimCreator/Graphics/CustomRenderingOptions.hpp>
#include <OpenSimCreator/Graphics/OverlayDecorationOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/AppSettingValue.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

#include <sstream>
#include <string_view>
#include <unordered_map>

namespace
{
    std::unordered_map<std::string, osc::AppSettingValue> ToValues(
        std::string_view prefix,
        osc::ModelRendererParams const& params)
    {
        std::unordered_map<std::string, osc::AppSettingValue> rv;
        std::string subPrefix;
        auto const callback = [&subPrefix, &rv](std::string_view subkey, osc::AppSettingValue value)
        {
            rv.insert_or_assign(subPrefix + std::string{subkey}, std::move(value));
        };

        subPrefix = std::string{prefix} + std::string{"decorations/"};
        params.decorationOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"overlays/"};
        params.overlayOptions.forEachOptionAsAppSettingValue(callback);
        subPrefix = std::string{prefix} + std::string{"graphics/"};
        params.renderingOptions.forEachOptionAsAppSettingValue(callback);
        // TODO: lightColor
        // TODO: backgroundColor
        // TODO: floorLocation

        return rv;
    }

    void UpdFromValues(
        std::string_view prefix,
        std::unordered_map<std::string, osc::AppSettingValue> const& values,
        osc::ModelRendererParams& params)
    {
        params.decorationOptions.tryUpdFromValues(std::string{prefix} + "decorations/", values);
        params.overlayOptions.tryUpdFromValues(std::string{prefix} + "overlays/", values);
        params.renderingOptions.tryUpdFromValues(std::string{prefix} + "graphics/", values);
        // TODO: lightColor
        // TODO: backgroundColor
        // TODO: floorLocation
    }
}

osc::ModelRendererParams::ModelRendererParams() :
    lightColor{SceneRendererParams::DefaultLightColor()},
    backgroundColor{SceneRendererParams::DefaultBackgroundColor()},
    floorLocation{SceneRendererParams::DefaultFloorLocation()},
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
        if (auto configV = config.getValue(k))
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

    for (auto const& [aK, aV] : aVals)
    {
        if (auto const it = bVals.find(aK); it != bVals.end())
        {
            if (it->second != aV)
            {
                config.setValue(it->first, it->second);
            }
        }
    }
}
