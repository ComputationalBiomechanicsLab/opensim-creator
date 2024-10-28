#include "MuscleSizingStyle.h"

#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleSizingStyleMetadata>({
        MuscleSizingStyleMetadata{
            "opensim",  // legacy behavior (changed to 'Fixed' in #933)
            "Fixed",
            MuscleSizingStyle::Fixed,
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

std::span<const MuscleSizingStyleMetadata> osc::GetAllMuscleSizingStyleMetadata()
{
    return c_Metadata;
}

const MuscleSizingStyleMetadata& osc::GetMuscleSizingStyleMetadata(MuscleSizingStyle s)
{
    return GetAllMuscleSizingStyleMetadata()[GetIndexOf(s)];
}

ptrdiff_t osc::GetIndexOf(MuscleSizingStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
