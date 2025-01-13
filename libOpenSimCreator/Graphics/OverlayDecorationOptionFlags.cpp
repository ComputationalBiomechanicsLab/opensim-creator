#include "OverlayDecorationOptionFlags.h"

#include <array>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<OverlayDecorationOptionFlagsMetadata>(
    {
        OverlayDecorationOptionFlagsMetadata
        {
            "show_xz_grid",
            "XZ Grid",
            OverlayDecorationOptionGroup::Alignment,
            OverlayDecorationOptionFlags::DrawXZGrid,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_xy_grid",
            "XY Grid",
            OverlayDecorationOptionGroup::Alignment,
            OverlayDecorationOptionFlags::DrawXYGrid,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_yz_grid",
            "YZ Grid",
            OverlayDecorationOptionGroup::Alignment,
            OverlayDecorationOptionFlags::DrawYZGrid,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_axis_lines",
            "Axis Lines",
            OverlayDecorationOptionGroup::Alignment,
            OverlayDecorationOptionFlags::DrawAxisLines,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_aabbs",
            "AABBs",
            OverlayDecorationOptionGroup::Development,
            OverlayDecorationOptionFlags::DrawAABBs,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_bvh",
            "BVH",
            OverlayDecorationOptionGroup::Development,
            OverlayDecorationOptionFlags::DrawBVH,
        },
    });
}

CStringView osc::getLabel(OverlayDecorationOptionGroup g)
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

std::span<const OverlayDecorationOptionFlagsMetadata> osc::GetAllOverlayDecorationOptionFlagsMetadata()
{
    return c_Metadata;
}
