#include "CustomRenderingOptions.hpp"

#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{
    enum class CustomRenderingOptionFlags : uint32_t {
        None              = 0,
        DrawFloor         = 1u << 0,
        MeshNormals       = 1u << 1,
        Shadows           = 1u << 2,
        DrawSelectionRims = 1u << 3,
        NUM_OPTIONS = 4,

        Default = DrawFloor | Shadows | DrawSelectionRims,
    };

    constexpr std::array<osc::CStringView, osc::NumOptions<CustomRenderingOptionFlags>()> c_CustomRenderingOptionLabels = osc::to_array<osc::CStringView>(
    {
        "Floor",
        "Mesh Normals",
        "Shadows",
        "Selection Rims",
    });

    enum class CustomRenderingOptionGroup {
        Rendering = 0,
        NUM_OPTIONS,
    };

    constexpr std::array<osc::CStringView, osc::NumOptions<CustomRenderingOptionGroup>()> c_CustomRenderingOptionGroupLabels = osc::to_array<osc::CStringView>(
    {
        "Rendering",
    });

    constexpr std::array<CustomRenderingOptionGroup, osc::NumOptions<CustomRenderingOptionFlags>()> c_CustomRenderingOptionGroups = osc::to_array<CustomRenderingOptionGroup>(
    {
        CustomRenderingOptionGroup::Rendering,
        CustomRenderingOptionGroup::Rendering,
        CustomRenderingOptionGroup::Rendering,
        CustomRenderingOptionGroup::Rendering,
    });

    void SetFlag(uint32_t& flags, CustomRenderingOptionFlags flag, bool v)
    {
        static_assert(std::is_same_v<std::underlying_type_t<CustomRenderingOptionFlags>, uint32_t>);

        if (v)
        {
            flags |= static_cast<std::underlying_type_t<CustomRenderingOptionFlags>>(flag);
        }
        else
        {
            flags &= ~static_cast<std::underlying_type_t<CustomRenderingOptionFlags>>(flag);
        }
    }

    bool GetFlag(uint32_t flags, CustomRenderingOptionFlags flag)
    {
        static_assert(std::is_same_v<std::underlying_type_t<CustomRenderingOptionFlags>, uint32_t>);

        return (flags & static_cast<uint32_t>(flag)) != 0u;
    }
}

osc::CustomRenderingOptions::CustomRenderingOptions() :
    m_Flags{static_cast<uint32_t>(CustomRenderingOptionFlags::Default)}
{
    static_assert(std::is_same_v<std::underlying_type_t<CustomRenderingOptionFlags>, std::decay_t<decltype(m_Flags)>>);
}

osc::CStringView osc::CustomRenderingOptions::getGroupLabel(ptrdiff_t i) const
{
    return c_CustomRenderingOptionGroupLabels.at(i);
}

size_t osc::CustomRenderingOptions::getNumOptions() const
{
    return osc::NumOptions<CustomRenderingOptionFlags>();
}

bool osc::CustomRenderingOptions::getOptionValue(ptrdiff_t i) const
{
    return (m_Flags & (1<<i)) != 0u;
}

void osc::CustomRenderingOptions::setOptionValue(ptrdiff_t i, bool v)
{
    OSC_ASSERT(static_cast<size_t>(i) <= osc::NumOptions<CustomRenderingOptionFlags>());

    SetFlag(m_Flags, static_cast<CustomRenderingOptionFlags>(1u<<i), v);
}

osc::CStringView osc::CustomRenderingOptions::getOptionLabel(ptrdiff_t i) const
{
    return c_CustomRenderingOptionLabels.at(i);
}

ptrdiff_t osc::CustomRenderingOptions::getOptionGroupIndex(ptrdiff_t i) const
{
    return static_cast<ptrdiff_t>(c_CustomRenderingOptionGroups.at(i));
}

bool osc::CustomRenderingOptions::getDrawFloor() const
{
    return GetFlag(m_Flags, CustomRenderingOptionFlags::DrawFloor);
}

void osc::CustomRenderingOptions::setDrawFloor(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags::DrawFloor, v);
}

bool osc::CustomRenderingOptions::getDrawMeshNormals() const
{
    return GetFlag(m_Flags, CustomRenderingOptionFlags::MeshNormals);
}

void osc::CustomRenderingOptions::setDrawMeshNormals(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags::MeshNormals, v);
}

bool osc::CustomRenderingOptions::getDrawShadows() const
{
    return GetFlag(m_Flags, CustomRenderingOptionFlags::Shadows);
}

void osc::CustomRenderingOptions::setDrawShadows(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags::Shadows, v);
}

bool osc::CustomRenderingOptions::getDrawSelectionRims() const
{
    return GetFlag(m_Flags, CustomRenderingOptionFlags::DrawSelectionRims);
}

void osc::CustomRenderingOptions::setDrawSelectionRims(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags::DrawSelectionRims, v);
}
