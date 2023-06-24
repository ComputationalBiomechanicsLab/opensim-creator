#include "CustomRenderingOptions.hpp"

#include <oscar/Utils/ArrayHelpers.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{
    // flags that toggle the viewer's behavior
    using CustomRenderingOptionFlags = uint32_t;
    enum CustomRenderingOptionFlags_ : uint32_t {
        CustomRenderingOptionFlags_None = 0,

        CustomRenderingOptionFlags_DrawFloor = 1 << 0,
        CustomRenderingOptionFlags_MeshNormals = 1 << 1,
        CustomRenderingOptionFlags_Shadows = 1 << 2,
        CustomRenderingOptionFlags_DrawSelectionRims = 1 << 3,

        CustomRenderingOptionFlags_COUNT = 4,
        CustomRenderingOptionFlags_Default =
            CustomRenderingOptionFlags_DrawFloor |
            CustomRenderingOptionFlags_Shadows |
            CustomRenderingOptionFlags_DrawSelectionRims,
    };

    auto constexpr c_CustomRenderingOptionLabels = osc::MakeSizedArray<osc::CStringView, static_cast<size_t>(CustomRenderingOptionFlags_COUNT)>
    (
        "Floor",
        "Mesh Normals",
        "Shadows",
        "Selection Rims"
    );

    enum class CustomRenderingOptionGroup : uint32_t {
        Rendering = 0,
        COUNT,
    };

    auto constexpr c_CustomRenderingOptionGroupLabels = osc::MakeSizedArray<osc::CStringView, static_cast<size_t>(CustomRenderingOptionGroup::COUNT)>
    (
        "Rendering"
    );

    auto constexpr c_CustomRenderingOptionGroups = osc::MakeSizedArray<CustomRenderingOptionGroup, static_cast<size_t>(CustomRenderingOptionFlags_COUNT)>
    (
        CustomRenderingOptionGroup::Rendering,
        CustomRenderingOptionGroup::Rendering,
        CustomRenderingOptionGroup::Rendering,
        CustomRenderingOptionGroup::Rendering
    );

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

osc::CustomRenderingOptions::CustomRenderingOptions() :
    m_Flags{static_cast<int32_t>(CustomRenderingOptionFlags_Default)}
{
    static_assert(std::is_same_v<std::underlying_type_t<CustomRenderingOptionFlags_>, std::decay_t<decltype(m_Flags)>>);
}

osc::CStringView osc::CustomRenderingOptions::getGroupLabel(ptrdiff_t i) const
{
    return c_CustomRenderingOptionGroupLabels.at(i);
}

size_t osc::CustomRenderingOptions::getNumOptions() const
{
    return static_cast<size_t>(CustomRenderingOptionFlags_COUNT);
}

bool osc::CustomRenderingOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & (1<<i);
}

void osc::CustomRenderingOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetFlag(m_Flags, 1<<i, v);
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
    return m_Flags & CustomRenderingOptionFlags_DrawFloor;
}

void osc::CustomRenderingOptions::setDrawFloor(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawFloor, v);
}

bool osc::CustomRenderingOptions::getDrawMeshNormals() const
{
    return m_Flags & CustomRenderingOptionFlags_MeshNormals;
}

void osc::CustomRenderingOptions::setDrawMeshNormals(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_MeshNormals, v);
}

bool osc::CustomRenderingOptions::getDrawShadows() const
{
    return m_Flags & CustomRenderingOptionFlags_Shadows;
}

void osc::CustomRenderingOptions::setDrawShadows(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_Shadows, v);
}

bool osc::CustomRenderingOptions::getDrawSelectionRims() const
{
    return m_Flags & CustomRenderingOptionFlags_DrawSelectionRims;
}

void osc::CustomRenderingOptions::setDrawSelectionRims(bool v)
{
    SetFlag(m_Flags, CustomRenderingOptionFlags_DrawSelectionRims, v);
}
