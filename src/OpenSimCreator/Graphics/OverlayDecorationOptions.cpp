#include "OverlayDecorationOptions.hpp"

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/SpanHelpers.hpp>

#include <cstddef>

size_t osc::OverlayDecorationOptions::getNumOptions() const
{
    return NumOptions<OverlayDecorationOptionFlags>();
}

bool osc::OverlayDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & At(GetAllOverlayDecorationOptionFlagsMetadata(), i).value;
}

void osc::OverlayDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(m_Flags, IthOption(i), v);
}

osc::CStringView osc::OverlayDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return At(GetAllOverlayDecorationOptionFlagsMetadata(), i).label;
}

osc::CStringView osc::OverlayDecorationOptions::getOptionGroupLabel(ptrdiff_t i) const
{
    return GetLabel(At(GetAllOverlayDecorationOptionFlagsMetadata(), i).group);
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
