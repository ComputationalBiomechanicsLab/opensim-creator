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
        auto const callback = [&prefix, &rv](std::string_view subkey, osc::AppSettingValue value)
        {
            std::string k{prefix};
            k += subkey;
            rv.insert_or_assign(k, std::move(value));
        };

        params.decorationOptions.forEachOptionAsAppSettingValue(callback);
        // TODO: overlayOptions
        // TODO: renderingOptions
        // TODO: lightColor
        // TODO: backgroundColor
        // TODO: floorLocation

        return rv;
    }

    void UpdFromValues(
        std::string_view,
        std::unordered_map<std::string, osc::AppSettingValue> const&,
        osc::ModelRendererParams&)
    {
        // TODO:
        //
        // - update `decorationOptions` (OpenSimDecorationOptions)
        // - update `overlayOptions` (OverlayDecorationOptions)
        // - update `renderingOptions` (CustomRenderingOptions)
        // - update `lightColor` (Color)
        // - update `backgroundColor` (Color)
        // - update `floorLocation` (glm::vec3)
        // - skip `camera`
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
