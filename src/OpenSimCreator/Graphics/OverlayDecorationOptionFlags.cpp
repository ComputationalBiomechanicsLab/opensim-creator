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
            OverlayDecorationOptionFlags::drawXZGrid,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_xy_grid",
            "XY Grid",
            OverlayDecorationOptionGroup::Alignment,
            OverlayDecorationOptionFlags::drawXYGrid,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_yz_grid",
            "YZ Grid",
            OverlayDecorationOptionGroup::Alignment,
            OverlayDecorationOptionFlags::drawYZGrid,
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
            OverlayDecorationOptionFlags::drawAABBs,
        },
        OverlayDecorationOptionFlagsMetadata
        {
            "show_bvh",
            "BVH",
            OverlayDecorationOptionGroup::Development,
            OverlayDecorationOptionFlags::drawBVH,
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
