#include "OverlayDecorationOptionFlags.hpp"

#include <nonstd/span.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>

namespace
{
    constexpr auto c_Metadata = osc::to_array<osc::OverlayDecorationOptionFlagsMetadata>(
    {
        osc::OverlayDecorationOptionFlagsMetadata
        {
            "show_xz_grid",
            "XZ Grid",
            osc::OverlayDecorationOptionGroup::Alignment,
            osc::OverlayDecorationOptionFlags::DrawXZGrid,
        },
        osc::OverlayDecorationOptionFlagsMetadata
        {
            "show_xy_grid",
            "XY Grid",
            osc::OverlayDecorationOptionGroup::Alignment,
            osc::OverlayDecorationOptionFlags::DrawXYGrid,
        },
        osc::OverlayDecorationOptionFlagsMetadata
        {
            "show_yz_grid",
            "YZ Grid",
            osc::OverlayDecorationOptionGroup::Alignment,
            osc::OverlayDecorationOptionFlags::DrawYZGrid,
        },
        osc::OverlayDecorationOptionFlagsMetadata
        {
            "show_axis_lines",
            "Axis Lines",
            osc::OverlayDecorationOptionGroup::Alignment,
            osc::OverlayDecorationOptionFlags::DrawAxisLines,
        },
        osc::OverlayDecorationOptionFlagsMetadata
        {
            "show_aabbs",
            "AABBs",
            osc::OverlayDecorationOptionGroup::Development,
            osc::OverlayDecorationOptionFlags::DrawAABBs,
        },
        osc::OverlayDecorationOptionFlagsMetadata
        {
            "show_bvh",
            "BVH",
            osc::OverlayDecorationOptionGroup::Development,
            osc::OverlayDecorationOptionFlags::DrawBVH,
        },
    });
}

osc::CStringView osc::GetLabel(OverlayDecorationOptionGroup g)
{
    switch (g)
    {
    case OverlayDecorationOptionGroup::Alignment:
        return "Alignment";
    case OverlayDecorationOptionGroup::Development:
    default:
        return "Development";
    }
}

nonstd::span<osc::OverlayDecorationOptionFlagsMetadata const> osc::GetAllOverlayDecorationOptionFlagsMetadata()
{
    return c_Metadata;
}
