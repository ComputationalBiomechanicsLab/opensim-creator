#include "MuscleSizingStyle.h"

#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleSizingStyleMetadata>(
    {
        MuscleSizingStyleMetadata
        {
            "opensim",
            "OpenSim",
            MuscleSizingStyle::OpenSim,
        },
        MuscleSizingStyleMetadata
        {
            "pcsa_derived",
            "PCSA-derived",
            MuscleSizingStyle::PcsaDerived,
        },
    });
    static_assert(c_Metadata.size() == num_options<MuscleSizingStyle>());
}

std::span<MuscleSizingStyleMetadata const> osc::GetAllMuscleSizingStyleMetadata()
{
    return c_Metadata;
}

MuscleSizingStyleMetadata const& osc::GetMuscleSizingStyleMetadata(MuscleSizingStyle s)
{
    return GetAllMuscleSizingStyleMetadata()[GetIndexOf(s)];
}

ptrdiff_t osc::GetIndexOf(MuscleSizingStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
