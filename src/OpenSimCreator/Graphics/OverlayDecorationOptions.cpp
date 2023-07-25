#include "OverlayDecorationOptions.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{
    // flags that toggle the viewer's behavior
    using CustomRenderingOptionFlags = uint32_t;
    enum CustomRenderingOptionFlags_ : uint32_t {
        CustomRenderingOptionFlags_None = 0,

        CustomRenderingOptionFlags_DrawXZGrid = 1 << 0,
        CustomRenderingOptionFlags_DrawXYGrid = 1 << 1,
        CustomRenderingOptionFlags_DrawYZGrid = 1 << 2,
        CustomRenderingOptionFlags_DrawAxisLines = 1 << 3,

        CustomRenderingOptionFlags_DrawAABBs = 1 << 4,
        CustomRenderingOptionFlags_DrawBVH = 1 << 5,

        CustomRenderingOptionFlags_COUNT = 6,
        CustomRenderingOptionFlags_Default = CustomRenderingOptionFlags_None,
    };

    constexpr auto c_CustomRenderingOptionLabels = osc::to_array<osc::CStringView>(
    {
        "XZ Grid",
        "XY Grid",
        "YZ Grid",
        "Axis Lines",

        "AABBs",
        "BVH",
    });
    static_assert(c_CustomRenderingOptionLabels.size() == static_cast<size_t>(CustomRenderingOptionFlags_COUNT));

    enum class CustomRenderingOptionGroup {
        Alignment,
        Development,
        NUM_OPTIONS,
    };

    constexpr auto c_CustomRenderingOptionGroupLabels = osc::to_array<osc::CStringView>(
    {
        "Alignment",
        "Development",
    });
    static_assert(c_CustomRenderingOptionGroupLabels.size() == osc::NumOptions<CustomRenderingOptionGroup>());

    constexpr auto c_CustomRenderingOptionGroups = osc::to_array<CustomRenderingOptionGroup>(
    {
        CustomRenderingOptionGroup::Alignment,
        CustomRenderingOptionGroup::Alignment,
        CustomRenderingOptionGroup::Alignment,
        CustomRenderingOptionGroup::Alignment,

        CustomRenderingOptionGroup::Development,
        CustomRenderingOptionGroup::Development,
    });
    static_assert(c_CustomRenderingOptionGroups.size() == static_cast<size_t>(CustomRenderingOptionFlags_COUNT));

    void SetFlag(uint32_t& flags, uint32_t flag, bool v)
    {
        if (v)
        {
            flags |= flag;
        }
        else
        {
            flags &= ~flag;
        }
    }
}

osc::OverlayDecorationOptions::OverlayDecorationOptions() :
    m_Flags{static_cast<int32_t>(CustomRenderingOptionFlags_Default)}
{
    static_assert(std::is_same_v<std::underlying_type_t<CustomRenderingOptionFlags_>, std::decay_t<decltype(m_Flags)>>);
}

osc::CStringView osc::OverlayDecorationOptions::getGroupLabel(ptrdiff_t i) const
{
    return c_CustomRenderingOptionGroupLabels.at(i);
}

size_t osc::OverlayDecorationOptions::getNumOptions() const
{
    return static_cast<size_t>(CustomRenderingOptionFlags_COUNT);
}

bool osc::OverlayDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & (1<<i);
}

void osc::OverlayDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetFlag(m_Flags, 1<<i, v);
}

osc::CStringView osc::OverlayDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return c_CustomRenderingOptionLabels.at(i);
}

ptrdiff_t osc::OverlayDecorationOptions::getOptionGroupIndex(ptrdiff_t i) const
{
    return static_cast<ptrdiff_t>(c_CustomRenderingOptionGroups.at(i));
}

bool osc::OverlayDecorationOptions::getDrawXZGrid() const
{
    return m_Flags & CustomRenderingOptionFlags_DrawXZGrid;
}

void osc::OverlayDecorationOptions::setDrawXZGrid(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawXZGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawXYGrid() const
{
    return m_Flags & CustomRenderingOptionFlags_DrawXYGrid;
}

void osc::OverlayDecorationOptions::setDrawXYGrid(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawXYGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawYZGrid() const
{
    return m_Flags & CustomRenderingOptionFlags_DrawYZGrid;
}

void osc::OverlayDecorationOptions::setDrawYZGrid(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawYZGrid, v);
}

bool osc::OverlayDecorationOptions::getDrawAxisLines() const
{
    return m_Flags & CustomRenderingOptionFlags_DrawAxisLines;
}

void osc::OverlayDecorationOptions::setDrawAxisLines(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawAxisLines, v);
}

bool osc::OverlayDecorationOptions::getDrawAABBs() const
{
    return m_Flags & CustomRenderingOptionFlags_DrawAABBs;
}

void osc::OverlayDecorationOptions::setDrawAABBs(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawAABBs, v);
}

bool osc::OverlayDecorationOptions::getDrawBVH() const
{
    return m_Flags & CustomRenderingOptionFlags_DrawBVH;
}

void osc::OverlayDecorationOptions::setDrawBVH(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawBVH, v);
}
