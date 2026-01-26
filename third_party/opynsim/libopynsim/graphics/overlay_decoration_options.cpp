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
    return m_Flags & osc::at(GetAllOverlayDecorationOptionFlagsMetadata(), i).value;
}

void opyn::OverlayDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(m_Flags, IthOption(i), v);
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
    return m_Flags & OverlayDecorationOptionFlags::DrawXZGrid;
}

void opyn::OverlayDecorationOptions::setDrawXZGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawXZGrid, v);
}

bool opyn::OverlayDecorationOptions::getDrawXYGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawXYGrid;
}

void opyn::OverlayDecorationOptions::setDrawXYGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawXYGrid, v);
}

bool opyn::OverlayDecorationOptions::getDrawYZGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawYZGrid;
}

void opyn::OverlayDecorationOptions::setDrawYZGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawYZGrid, v);
}

bool opyn::OverlayDecorationOptions::getDrawAxisLines() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawAxisLines;
}

void opyn::OverlayDecorationOptions::setDrawAxisLines(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawAxisLines, v);
}

bool opyn::OverlayDecorationOptions::getDrawAABBs() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawAABBs;
}

void opyn::OverlayDecorationOptions::setDrawAABBs(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawAABBs, v);
}

bool opyn::OverlayDecorationOptions::getDrawBVH() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawBVH;
}

void opyn::OverlayDecorationOptions::setDrawBVH(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawBVH, v);
}

void opyn::OverlayDecorationOptions::forEachOptionAsAppSettingValue(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    for (const auto& metadata : GetAllOverlayDecorationOptionFlagsMetadata()) {
        callback(metadata.id, osc::Variant{m_Flags & metadata.value});
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
            SetOption(m_Flags, metadata.value, to<bool>(*v));
        }
    }
}
