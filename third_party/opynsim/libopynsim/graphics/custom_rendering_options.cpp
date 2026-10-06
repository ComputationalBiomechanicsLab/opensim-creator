#include "custom_rendering_options.h"

#include <libopynsim/graphics/custom_rendering_option_flags.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/variant/variant.h>
#include <liboscar/variant/variant_type.h>

#include <cstddef>

size_t opyn::CustomRenderingOptions::getNumOptions() const
{
    return osc::num_flags<CustomRenderingOptionFlags>();
}

bool opyn::CustomRenderingOptions::getOptionValue(ptrdiff_t i) const
{
    return flags_ & CustomRenderingIthOption(i);
}

void opyn::CustomRenderingOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(flags_, CustomRenderingIthOption(i), v);
}

osc::CStringView opyn::CustomRenderingOptions::getOptionLabel(ptrdiff_t i) const
{
    return osc::at(GetAllCustomRenderingOptionFlagsMetadata(), i).label;
}

bool opyn::CustomRenderingOptions::getDrawFloor() const
{
    return flags_ & CustomRenderingOptionFlags::DrawFloor;
}

void opyn::CustomRenderingOptions::setDrawFloor(bool v)
{
    SetOption(flags_, CustomRenderingOptionFlags::DrawFloor, v);
}

bool opyn::CustomRenderingOptions::getDrawMeshNormals() const
{
    return flags_ & CustomRenderingOptionFlags::MeshNormals;
}

void opyn::CustomRenderingOptions::setDrawMeshNormals(bool v)
{
    SetOption(flags_, CustomRenderingOptionFlags::MeshNormals, v);
}

bool opyn::CustomRenderingOptions::getDrawShadows() const
{
    return flags_ & CustomRenderingOptionFlags::Shadows;
}

void opyn::CustomRenderingOptions::setDrawShadows(bool v)
{
    SetOption(flags_, CustomRenderingOptionFlags::Shadows, v);
}

bool opyn::CustomRenderingOptions::getDrawSelectionRims() const
{
    return flags_ & CustomRenderingOptionFlags::DrawSelectionRims;
}

void opyn::CustomRenderingOptions::setDrawSelectionRims(bool v)
{
    SetOption(flags_, CustomRenderingOptionFlags::DrawSelectionRims, v);
}

bool opyn::CustomRenderingOptions::getOrderIndependentTransparency() const
{
    return flags_ & CustomRenderingOptionFlags::OrderIndependentTransparency;
}

void opyn::CustomRenderingOptions::setOrderIndependentTransparency(bool v)
{
    SetOption(flags_, CustomRenderingOptionFlags::OrderIndependentTransparency, v);
}

void opyn::CustomRenderingOptions::forEachOptionAsAppSettingValue(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    for (const auto& metadata : GetAllCustomRenderingOptionFlagsMetadata()) {
        callback(metadata.id, osc::Variant{flags_ & metadata.value});
    }
}

void opyn::CustomRenderingOptions::tryUpdFromValues(
    std::string_view keyPrefix,
    const std::unordered_map<std::string, osc::Variant>& lut)
{
    for (const auto& metadata : GetAllCustomRenderingOptionFlagsMetadata()) {

        const std::string key = std::string{keyPrefix} + metadata.id;
        if (const auto* v = lookup_or_nullptr(lut, key); v and v->type() == osc::VariantType::Bool) {
            SetOption(flags_, metadata.value, to<bool>(*v));
        }
    }
}

void opyn::CustomRenderingOptions::applyTo(osc::SceneRendererParams& params) const
{
    params.draw_floor = getDrawFloor();
    params.draw_rims = getDrawSelectionRims();
    params.draw_mesh_normals = getDrawMeshNormals();
    params.draw_shadows = getDrawShadows();
    params.order_independent_transparency = getOrderIndependentTransparency();
}
