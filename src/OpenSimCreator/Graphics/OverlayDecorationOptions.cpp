#include "OverlayDecorationOptions.h"

#include <oscar/Platform/AppSettingValue.h>
#include <oscar/Platform/AppSettingValueType.h>
#include <oscar/Utils/At.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>

using namespace osc;

size_t osc::OverlayDecorationOptions::getNumOptions() const
{
    return NumFlags<OverlayDecorationOptionFlags>();
}

bool osc::OverlayDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & At(GetAllOverlayDecorationOptionFlagsMetadata(), i).value;
}

void osc::OverlayDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(m_Flags, IthOption(i), v);
}

CStringView osc::OverlayDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return At(GetAllOverlayDecorationOptionFlagsMetadata(), i).label;
}

CStringView osc::OverlayDecorationOptions::getOptionGroupLabel(ptrdiff_t i) const
{
    return getLabel(At(GetAllOverlayDecorationOptionFlagsMetadata(), i).group);
}

bool osc::OverlayDecorationOptions::getDrawXZGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawXZGrid;
}

void osc::OverlayDecorationOptions::setDrawXZGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawXZGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawXYGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawXYGrid;
}

void osc::OverlayDecorationOptions::setDrawXYGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawXYGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawYZGrid() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawYZGrid;
}

void osc::OverlayDecorationOptions::setDrawYZGrid(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawYZGrid, v);
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
    return m_Flags & OverlayDecorationOptionFlags::DrawAABBs;
}

void osc::OverlayDecorationOptions::setDrawAABBs(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawAABBs, v);
}

bool osc::OverlayDecorationOptions::getDrawBVH() const
{
    return m_Flags & OverlayDecorationOptionFlags::DrawBVH;
}

void osc::OverlayDecorationOptions::setDrawBVH(bool v)
{
    SetOption(m_Flags, OverlayDecorationOptionFlags::DrawBVH, v);
}

void osc::OverlayDecorationOptions::forEachOptionAsAppSettingValue(std::function<void(std::string_view, AppSettingValue const&)> const& callback) const
{
    for (auto const& metadata : GetAllOverlayDecorationOptionFlagsMetadata())
    {
        callback(metadata.id, AppSettingValue{m_Flags & metadata.value});
    }
}

void osc::OverlayDecorationOptions::tryUpdFromValues(std::string_view keyPrefix, std::unordered_map<std::string, AppSettingValue> const& lut)
{
    for (size_t i = 0; i < NumFlags<OverlayDecorationOptionFlags>(); ++i)
    {
        auto const& metadata = At(GetAllOverlayDecorationOptionFlagsMetadata(), i);
        if (auto const it = lut.find(std::string{keyPrefix}+metadata.id); it != lut.end() && it->second.type() == osc::AppSettingValueType::Bool)
        {
            SetOption(m_Flags, metadata.value, it->second.toBool());
        }
    }
}
