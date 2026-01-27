#include "muscle_sizing_style.h"

#include <liboscar/utilities/enum_helpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace opyn;

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
    static_assert(c_Metadata.size() == osc::num_options<MuscleSizingStyle>());
}

std::span<const MuscleSizingStyleMetadata> opyn::GetAllMuscleSizingStyleMetadata()
{
    return c_Metadata;
}

const MuscleSizingStyleMetadata& opyn::GetMuscleSizingStyleMetadata(MuscleSizingStyle s)
{
    return GetAllMuscleSizingStyleMetadata()[GetIndexOf(s)];
}

ptrdiff_t opyn::GetIndexOf(MuscleSizingStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
