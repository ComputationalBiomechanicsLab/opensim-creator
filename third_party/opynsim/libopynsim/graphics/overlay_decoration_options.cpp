#include "overlay_decoration_options.h"

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/variant/variant.h>
#include <liboscar/variant/variant_type.h>

#include <cstddef>

size_t opyn::OverlayDecorationOptions::getNumOptions() const
{
    return osc::num_flags<OverlayDecorationOptionFlags>();
}

bool opyn::OverlayDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return flags_ & osc::at(GetAllOverlayDecorationOptionFlagsMetadata(), i).value;
}

void opyn::OverlayDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(flags_, IthOption(i), v);
}

osc::CStringView opyn::OverlayDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return osc::at(GetAllOverlayDecorationOptionFlagsMetadata(), i).label;
}

osc::CStringView opyn::OverlayDecorationOptions::getOptionGroupLabel(ptrdiff_t i) const
{
    return getLabel(osc::at(GetAllOverlayDecorationOptionFlagsMetadata(), i).group);
}

bool opyn::OverlayDecorationOptions::getDrawXZGrid() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawXZGrid;
}

void opyn::OverlayDecorationOptions::setDrawXZGrid(bool v)
{
    SetOption(flags_, OverlayDecorationOptionFlags::DrawXZGrid, v);
}

bool opyn::OverlayDecorationOptions::getDrawXYGrid() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawXYGrid;
}

void opyn::OverlayDecorationOptions::setDrawXYGrid(bool v)
{
    SetOption(flags_, OverlayDecorationOptionFlags::DrawXYGrid, v);
}

bool opyn::OverlayDecorationOptions::getDrawYZGrid() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawYZGrid;
}

void opyn::OverlayDecorationOptions::setDrawYZGrid(bool v)
{
    SetOption(flags_, OverlayDecorationOptionFlags::DrawYZGrid, v);
}

bool opyn::OverlayDecorationOptions::getDrawAxisLines() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawAxisLines;
}

void opyn::OverlayDecorationOptions::setDrawAxisLines(bool v)
{
    SetOption(flags_, OverlayDecorationOptionFlags::DrawAxisLines, v);
}

bool opyn::OverlayDecorationOptions::getDrawAABBs() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawAABBs;
}

void opyn::OverlayDecorationOptions::setDrawAABBs(bool v)
{
    SetOption(flags_, OverlayDecorationOptionFlags::DrawAABBs, v);
}

bool opyn::OverlayDecorationOptions::getDrawBVH() const
{
    return flags_ & OverlayDecorationOptionFlags::DrawBVH;
}

void opyn::OverlayDecorationOptions::setDrawBVH(bool v)
{
    SetOption(flags_, OverlayDecorationOptionFlags::DrawBVH, v);
}

void opyn::OverlayDecorationOptions::forEachOptionAsAppSettingValue(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    for (const auto& metadata : GetAllOverlayDecorationOptionFlagsMetadata()) {
        callback(metadata.id, osc::Variant{flags_ & metadata.value});
    }
}

void opyn::OverlayDecorationOptions::tryUpdFromValues(
    std::string_view keyPrefix,
    const std::unordered_map<std::string, osc::Variant>& lut)
{
    for (size_t i = 0; i < osc::num_flags<OverlayDecorationOptionFlags>(); ++i)
    {
        const auto& metadata = osc::at(GetAllOverlayDecorationOptionFlagsMetadata(), i);

        const std::string key = std::string{keyPrefix}+metadata.id;
        if (const auto* v = lookup_or_nullptr(lut, key); v and v->type() == osc::VariantType::Bool) {
            SetOption(flags_, metadata.value, to<bool>(*v));
        }
    }
}
