#include "CustomRenderingOptions.h"

#include <OpenSimCreator/Graphics/CustomRenderingOptionFlags.h>

#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>
#include <cstdint>

using namespace osc;

size_t osc::CustomRenderingOptions::getNumOptions() const
{
    return NumFlags<CustomRenderingOptionFlags>();
}

bool osc::CustomRenderingOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & CustomRenderingIthOption(i);
}

void osc::CustomRenderingOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetOption(m_Flags, CustomRenderingIthOption(i), v);
}

CStringView osc::CustomRenderingOptions::getOptionLabel(ptrdiff_t i) const
{
    return at(GetAllCustomRenderingOptionFlagsMetadata(), i).label;
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

void osc::CustomRenderingOptions::forEachOptionAsAppSettingValue(std::function<void(std::string_view, AppSettingValue const&)> const& callback) const
{
    for (auto const& metadata : GetAllCustomRenderingOptionFlagsMetadata())
    {
        callback(metadata.id, AppSettingValue{m_Flags & metadata.value});
    }
}

void osc::CustomRenderingOptions::tryUpdFromValues(std::string_view keyPrefix, std::unordered_map<std::string, AppSettingValue> const& lut)
{
    for (auto const& metadata : GetAllCustomRenderingOptionFlagsMetadata()) {

        std::string key = std::string{keyPrefix} + metadata.id;
        if (auto const* v = lookup_or_nullptr(lut, key); v->type() == AppSettingValueType::Bool) {
            SetOption(m_Flags, metadata.value, v->to_bool());
        }
    }
}
