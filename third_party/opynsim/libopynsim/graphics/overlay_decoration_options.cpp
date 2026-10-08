#include "overlay_decoration_options.h"

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/variant/variant.h>
#include <liboscar/variant/variant_type.h>

#include <cstddef>

size_t opyn::OverlayDecorationOptions::get_num_options() const
{
    return osc::num_flags<OverlayDecorationOptionFlags>();
}

bool opyn::OverlayDecorationOptions::get_option_value(ptrdiff_t i) const
{
    return flags_ & osc::at(get_all_overlay_decoration_option_flags_metadata(), i).value;
}

void opyn::OverlayDecorationOptions::set_option_value(ptrdiff_t i, bool v)
{
    set_option(flags_, ith_option(i), v);
}

osc::CStringView opyn::OverlayDecorationOptions::get_option_label(ptrdiff_t i) const
{
    return osc::at(get_all_overlay_decoration_option_flags_metadata(), i).label;
}

osc::CStringView opyn::OverlayDecorationOptions::get_option_group_label(ptrdiff_t i) const
{
    return get_label(osc::at(get_all_overlay_decoration_option_flags_metadata(), i).group);
}

bool opyn::OverlayDecorationOptions::get_draw_xz_grid() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawXZGrid;
}

void opyn::OverlayDecorationOptions::set_draw_xz_grid(bool v)
{
    set_option(flags_, OverlayDecorationOptionFlags::DrawXZGrid, v);
}

bool opyn::OverlayDecorationOptions::get_draw_xy_grid() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawXYGrid;
}

void opyn::OverlayDecorationOptions::set_draw_xy_grid(bool v)
{
    set_option(flags_, OverlayDecorationOptionFlags::DrawXYGrid, v);
}

bool opyn::OverlayDecorationOptions::get_draw_yz_grid() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawYZGrid;
}

void opyn::OverlayDecorationOptions::set_draw_yz_grid(bool v)
{
    set_option(flags_, OverlayDecorationOptionFlags::DrawYZGrid, v);
}

bool opyn::OverlayDecorationOptions::get_draw_axis_lines() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawAxisLines;
}

void opyn::OverlayDecorationOptions::set_draw_axis_lines(bool v)
{
    set_option(flags_, OverlayDecorationOptionFlags::DrawAxisLines, v);
}

bool opyn::OverlayDecorationOptions::get_draw_aabbs() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawAABBs;
}

void opyn::OverlayDecorationOptions::set_draw_aabbs(bool v)
{
    set_option(flags_, OverlayDecorationOptionFlags::DrawAABBs, v);
}

bool opyn::OverlayDecorationOptions::get_draw_bvh() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawBVH;
}

void opyn::OverlayDecorationOptions::set_draw_bvh(bool v)
{
    set_option(flags_, OverlayDecorationOptionFlags::DrawBVH, v);
}

void opyn::OverlayDecorationOptions::for_each_option_as_app_setting_value(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    for (const auto& metadata : get_all_overlay_decoration_option_flags_metadata()) {
        callback(metadata.id, osc::Variant{flags_ & metadata.value});
    }
}

void opyn::OverlayDecorationOptions::try_upd_from_values(
    std::string_view key_prefix,
    const std::unordered_map<std::string, osc::Variant>& lut)
{
    for (size_t i = 0; i < osc::num_flags<OverlayDecorationOptionFlags>(); ++i)
    {
        const auto& metadata = osc::at(get_all_overlay_decoration_option_flags_metadata(), i);

        const std::string key = std::string{key_prefix}+metadata.id;
        if (const auto* v = lookup_or_nullptr(lut, key); v and v->type() == osc::VariantType::Bool) {
            set_option(flags_, metadata.value, to<bool>(*v));
        }
    }
}
