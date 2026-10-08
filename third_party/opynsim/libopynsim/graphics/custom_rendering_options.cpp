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

size_t opyn::CustomRenderingOptions::get_num_options() const
{
    return osc::num_flags<CustomRenderingOptionFlags>();
}

bool opyn::CustomRenderingOptions::get_option_value(ptrdiff_t i) const
{
    return flags_ & custom_rendering_ith_option(i);
}

void opyn::CustomRenderingOptions::set_option_value(ptrdiff_t i, bool v)
{
    set_option(flags_, custom_rendering_ith_option(i), v);
}

osc::CStringView opyn::CustomRenderingOptions::get_option_label(ptrdiff_t i) const
{
    return osc::at(get_all_custom_rendering_option_flags_metadata(), i).label;
}

bool opyn::CustomRenderingOptions::get_draw_floor() const
{
    return flags_ & CustomRenderingOptionFlags::DrawFloor;
}

void opyn::CustomRenderingOptions::set_draw_floor(bool v)
{
    set_option(flags_, CustomRenderingOptionFlags::DrawFloor, v);
}

bool opyn::CustomRenderingOptions::get_draw_mesh_normals() const
{
    return flags_ & CustomRenderingOptionFlags::MeshNormals;
}

void opyn::CustomRenderingOptions::set_draw_mesh_normals(bool v)
{
    set_option(flags_, CustomRenderingOptionFlags::MeshNormals, v);
}

bool opyn::CustomRenderingOptions::get_draw_shadows() const
{
    return flags_ & CustomRenderingOptionFlags::Shadows;
}

void opyn::CustomRenderingOptions::set_draw_shadows(bool v)
{
    set_option(flags_, CustomRenderingOptionFlags::Shadows, v);
}

bool opyn::CustomRenderingOptions::get_draw_selection_rims() const
{
    return flags_ & CustomRenderingOptionFlags::DrawSelectionRims;
}

void opyn::CustomRenderingOptions::set_draw_selection_rims(bool v)
{
    set_option(flags_, CustomRenderingOptionFlags::DrawSelectionRims, v);
}

bool opyn::CustomRenderingOptions::get_order_independent_transparency() const
{
    return flags_ & CustomRenderingOptionFlags::OrderIndependentTransparency;
}

void opyn::CustomRenderingOptions::set_order_independent_transparency(bool v)
{
    set_option(flags_, CustomRenderingOptionFlags::OrderIndependentTransparency, v);
}

void opyn::CustomRenderingOptions::for_each_option_as_app_setting_value(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    for (const auto& metadata : get_all_custom_rendering_option_flags_metadata()) {
        callback(metadata.id, osc::Variant{flags_ & metadata.value});
    }
}

void opyn::CustomRenderingOptions::try_upd_from_values(
    std::string_view key_prefix,
    const std::unordered_map<std::string, osc::Variant>& lut)
{
    for (const auto& metadata : get_all_custom_rendering_option_flags_metadata()) {

        const std::string key = std::string{key_prefix} + metadata.id;
        if (const auto* v = lookup_or_nullptr(lut, key); v and v->type() == osc::VariantType::Bool) {
            set_option(flags_, metadata.value, to<bool>(*v));
        }
    }
}

void opyn::CustomRenderingOptions::apply_to(osc::SceneRendererParams& params) const
{
    params.draw_floor = get_draw_floor();
    params.draw_rims = get_draw_selection_rims();
    params.draw_mesh_normals = get_draw_mesh_normals();
    params.draw_shadows = get_draw_shadows();
    params.order_independent_transparency = get_order_independent_transparency();
}
