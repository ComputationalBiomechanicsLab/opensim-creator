#include "MuscleSizingStyle.hpp"

#include <nonstd/span.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <cstddef>

namespace
{
    constexpr auto c_Metadata = osc::to_array<osc::MuscleSizingStyleMetadata>(
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

nonstd::span<osc::MuscleSizingStyleMetadata const> osc::GetAllMuscleSizingStyleMetadata()
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
