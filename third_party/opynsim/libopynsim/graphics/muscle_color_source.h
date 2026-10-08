#pragma once

#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <span>

namespace opyn
{
    enum class MuscleColorSource {
        AppearanceProperty,
        Activation,
        Excitation,
        Force,
        FiberLength,
        NUM_OPTIONS,

        Default = Activation,
    };

    struct MuscleColorSourceMetadata final {
        osc::CStringView id;
        osc::CStringView label;
        MuscleColorSource value;
    };
    std::span<const MuscleColorSourceMetadata> get_all_possible_muscle_coloring_sources_metadata();
    const MuscleColorSourceMetadata& get_muscle_coloring_style_metadata(MuscleColorSource);
    ptrdiff_t get_index_of(MuscleColorSource);
}
