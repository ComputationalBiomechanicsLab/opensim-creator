#include "CustomRenderingOptions.h"

#include <libopensimcreator/Graphics/CustomRenderingOptionFlags.h>

#include <liboscar/graphics/scene/SceneRendererParams.h>
#include <liboscar/utils/Algorithms.h>
#include <liboscar/utils/Conversion.h>
#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/EnumHelpers.h>
#include <liboscar/variant/Variant.h>
#include <liboscar/variant/VariantType.h>

#include <cstddef>

using namespace osc;

size_t osc::CustomRenderingOptions::getNumOptions() const
{
    return num_flags<CustomRenderingOptionFlags>();
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

bool osc::CustomRenderingOptions::getOrderIndependentTransparency() const
{
    return m_Flags & CustomRenderingOptionFlags::OrderIndependentTransparency;
}

void osc::CustomRenderingOptions::setOrderIndependentTransparency(bool v)
{
    SetOption(m_Flags, CustomRenderingOptionFlags::OrderIndependentTransparency, v);
}

void osc::CustomRenderingOptions::forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const Variant&)>& callback) const
{
    for (const auto& metadata : GetAllCustomRenderingOptionFlagsMetadata()) {
        callback(metadata.id, Variant{m_Flags & metadata.value});
    }
}

void osc::CustomRenderingOptions::tryUpdFromValues(std::string_view keyPrefix, const std::unordered_map<std::string, Variant>& lut)
{
    for (const auto& metadata : GetAllCustomRenderingOptionFlagsMetadata()) {

        const std::string key = std::string{keyPrefix} + metadata.id;
        if (const auto* v = lookup_or_nullptr(lut, key); v and v->type() == VariantType::Bool) {
            SetOption(m_Flags, metadata.value, to<bool>(*v));
        }
    }
}

void osc::CustomRenderingOptions::applyTo(SceneRendererParams& params) const
{
    params.draw_floor = getDrawFloor();
    params.draw_rims = getDrawSelectionRims();
    params.draw_mesh_normals = getDrawMeshNormals();
    params.draw_shadows = getDrawShadows();
    params.order_independent_transparency = getOrderIndependentTransparency();
}
