#include "CustomRenderingOptionFlags.hpp"

#include <array>
#include <span>

namespace
{
    constexpr auto c_Metadata = std::to_array<osc::CustomRenderingOptionFlagsMetadata>(
    {
        osc::CustomRenderingOptionFlagsMetadata
        {
            "show_floor",
            "Floor",
            osc::CustomRenderingOptionFlags::DrawFloor,
        },
        osc::CustomRenderingOptionFlagsMetadata
        {
            "show_mesh_normals",
            "Mesh Normals",
            osc::CustomRenderingOptionFlags::MeshNormals,
        },
        osc::CustomRenderingOptionFlagsMetadata
        {
            "show_shadows",
            "Shadows",
            osc::CustomRenderingOptionFlags::Shadows,
        },
        osc::CustomRenderingOptionFlagsMetadata
        {
            "show_selection_rims",
            "Selection Rims",
            osc::CustomRenderingOptionFlags::DrawSelectionRims,
        },
    });
}

std::span<osc::CustomRenderingOptionFlagsMetadata const> osc::GetAllCustomRenderingOptionFlagsMetadata()
{
    return c_Metadata;
}
