#include "custom_rendering_option_flags.h"

#include <array>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<CustomRenderingOptionFlagsMetadata>(
    {
        CustomRenderingOptionFlagsMetadata
        {
            "show_floor",
            "Floor",
            CustomRenderingOptionFlags::DrawFloor,
        },
        CustomRenderingOptionFlagsMetadata
        {
            "show_mesh_normals",
            "Mesh Normals",
            CustomRenderingOptionFlags::MeshNormals,
        },
        CustomRenderingOptionFlagsMetadata
        {
            "show_shadows",
            "Shadows",
            CustomRenderingOptionFlags::Shadows,
        },
        CustomRenderingOptionFlagsMetadata
        {
            "show_selection_rims",
            "Selection Rims",
            CustomRenderingOptionFlags::DrawSelectionRims,
        },
        CustomRenderingOptionFlagsMetadata
        {
            "order_independent_transparency",
            "Order-Independent Transparency",
            CustomRenderingOptionFlags::OrderIndependentTransparency,
        },
    });
}

std::span<const CustomRenderingOptionFlagsMetadata> osc::GetAllCustomRenderingOptionFlagsMetadata()
{
    return c_Metadata;
}
