#include "CustomRenderingOptions.hpp"

#include <OpenSimCreator/Graphics/CustomRenderingOptionFlags.hpp>

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/SpanHelpers.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

size_t osc::CustomRenderingOptions::getNumOptions() const
{
    return osc::NumOptions<CustomRenderingOptionFlags>();
}

bool osc::CustomRenderingOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & CustomRenderingIthOption(i);
}

void osc::CustomRenderingOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(m_Flags, CustomRenderingIthOption(i), v);
}

osc::CStringView osc::CustomRenderingOptions::getOptionLabel(ptrdiff_t i) const
{
    return At(GetAllCustomRenderingOptionFlagsMetadata(), i).label;
}

bool osc::CustomRenderingOptions::getDrawFloor() const
{
    return m_Flags & CustomRenderingOptionFlags::DrawFloor;
}

void osc::CustomRenderingOptions::setDrawFloor(bool v)
{
    SetOption(m_Flags, CustomRenderingOptionFlags::DrawFloor, v);
}

bool osc::CustomRenderingOptions::getDrawMeshNormals() const
{
    return m_Flags & CustomRenderingOptionFlags::MeshNormals;
}

void osc::CustomRenderingOptions::setDrawMeshNormals(bool v)
{
    SetOption(m_Flags, CustomRenderingOptionFlags::MeshNormals, v);
}

bool osc::CustomRenderingOptions::getDrawShadows() const
{
    return m_Flags & CustomRenderingOptionFlags::Shadows;
}

void osc::CustomRenderingOptions::setDrawShadows(bool v)
{
    SetOption(m_Flags, CustomRenderingOptionFlags::Shadows, v);
}

bool osc::CustomRenderingOptions::getDrawSelectionRims() const
{
    return m_Flags & CustomRenderingOptionFlags::DrawSelectionRims;
}

void osc::CustomRenderingOptions::setDrawSelectionRims(bool v)
{
    SetOption(m_Flags, CustomRenderingOptionFlags::DrawSelectionRims, v);
}
