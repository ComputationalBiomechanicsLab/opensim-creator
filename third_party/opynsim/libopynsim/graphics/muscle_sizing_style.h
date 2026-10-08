#pragma once

#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <span>

namespace opyn
{
    enum class MuscleSizingStyle {
        Fixed,
        PcsaDerived,
        NUM_OPTIONS,

        Default = Fixed,
    };

    struct MuscleSizingStyleMetadata final {
        osc::CStringView id;
        osc::CStringView label;
        MuscleSizingStyle value;
    };
    std::span<const MuscleSizingStyleMetadata> get_all_muscle_sizing_style_metadata();
    const MuscleSizingStyleMetadata& get_muscle_sizing_style_metadata(MuscleSizingStyle);
    ptrdiff_t get_index_of(MuscleSizingStyle);
}
