#include "OverlayDecorationOptions.h"

#include <oscar/Platform/AppSettingValue.h>
#include <oscar/Platform/AppSettingValueType.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>

using namespace osc;

size_t osc::OverlayDecorationOptions::getNumOptions() const
{
    return num_flags<OverlayDecorationOptionFlags>();
}

bool osc::OverlayDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & at(GetAllOverlayDecorationOptionFlagsMetadata(), i).value;
}

void osc::OverlayDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(m_Flags, IthOption(i), v);
}

CStringView osc::OverlayDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return at(GetAllOverlayDecorationOptionFlagsMetadata(), i).label;
}

CStringView osc::OverlayDecorationOptions::getOptionGroupLabel(ptrdiff_t i) const
{
    return getLabel(at(GetAllOverlayDecorationOptionFlagsMetadata(), i).group);
}

bool osc::OverlayDecorationOptions::getDrawXZGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::drawXZGrid;
}

void osc::OverlayDecorationOptions::setDrawXZGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::drawXZGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawXYGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::drawXYGrid;
}

void osc::OverlayDecorationOptions::setDrawXYGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::drawXYGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawYZGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::drawYZGrid;
}

void osc::OverlayDecorationOptions::setDrawYZGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::drawYZGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawAxisLines() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawAxisLines;
}

void osc::OverlayDecorationOptions::setDrawAxisLines(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawAxisLines, v);
}

bool osc::OverlayDecorationOptions::getDrawAABBs() const
{
    return m_Flags & OverlayDecorationOptionFlags::drawAABBs;
}

void osc::OverlayDecorationOptions::setDrawAABBs(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::drawAABBs, v);
}

bool osc::OverlayDecorationOptions::getDrawBVH() const
{
    return m_Flags & OverlayDecorationOptionFlags::drawBVH;
}

void osc::OverlayDecorationOptions::setDrawBVH(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::drawBVH, v);
}

void osc::OverlayDecorationOptions::forEachOptionAsAppSettingValue(std::function<void(std::string_view, const AppSettingValue&)> const& callback) const
{
    for (const auto& metadata : GetAllOverlayDecorationOptionFlagsMetadata())
    {
        callback(metadata.id, AppSettingValue{m_Flags & metadata.value});
    }
}

void osc::OverlayDecorationOptions::tryUpdFromValues(std::string_view keyPrefix, std::unordered_map<std::string, AppSettingValue> const& lut)
{
    for (size_t i = 0; i < num_flags<OverlayDecorationOptionFlags>(); ++i)
    {
        const auto& metadata = at(GetAllOverlayDecorationOptionFlagsMetadata(), i);

        const std::string key = std::string{keyPrefix}+metadata.id;
        if (const auto* v = lookup_or_nullptr(lut, key); v and v->type() == osc::AppSettingValueType::Bool) {
            SetOption(m_Flags, metadata.value, v->to_bool());
        }
    }
}
