#include "muscle_sizing_style.h"

#include <liboscar/utilities/enum_helpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace opyn;

namespace
{
    constexpr auto c_metadata = std::to_array<MuscleSizingStyleMetadata>({
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
    static_assert(c_metadata.size() == osc::num_options<MuscleSizingStyle>());
}

std::span<const MuscleSizingStyleMetadata> opyn::get_all_muscle_sizing_style_metadata()
{
    return c_metadata;
}

const MuscleSizingStyleMetadata& opyn::get_muscle_sizing_style_metadata(MuscleSizingStyle s)
{
    return get_all_muscle_sizing_style_metadata()[get_index_of(s)];
}

ptrdiff_t opyn::get_index_of(MuscleSizingStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
