#include "MuscleSizingStyle.hpp"

#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <cstddef>
#include <span>

namespace
{
    constexpr auto c_Metadata = std::to_array<osc::MuscleSizingStyleMetadata>(
    {
        osc::MuscleSizingStyleMetadata
        {
            "opensim",
            "OpenSim",
            osc::MuscleSizingStyle::OpenSim,
        },
        osc::MuscleSizingStyleMetadata
        {
            "pcsa_derived",
            "PCSA-derived",
            osc::MuscleSizingStyle::PcsaDerived,
        },
    });
    static_assert(c_Metadata.size() == osc::NumOptions<osc::MuscleSizingStyle>());
}

std::span<osc::MuscleSizingStyleMetadata const> osc::GetAllMuscleSizingStyleMetadata()
{
    return c_Metadata;
}

osc::MuscleSizingStyleMetadata const& osc::GetMuscleSizingStyleMetadata(MuscleSizingStyle s)
{
    return GetAllMuscleSizingStyleMetadata()[GetIndexOf(s)];
}

ptrdiff_t osc::GetIndexOf(MuscleSizingStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
