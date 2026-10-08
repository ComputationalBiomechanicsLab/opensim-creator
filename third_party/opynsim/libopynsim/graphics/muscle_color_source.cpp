#include "muscle_color_source.h"

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace opyn;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleColorSourceMetadata>({
        MuscleColorSourceMetadata{
            "opensim_appearance",
            "Appearance Property",
            MuscleColorSource::AppearanceProperty,
        },
        MuscleColorSourceMetadata{
            "activation",
            "Activation",
            MuscleColorSource::Activation,
        },
        MuscleColorSourceMetadata{
            "excitation",
            "Excitation",
            MuscleColorSource::Excitation,
        },
        MuscleColorSourceMetadata{
            "force",
            "Force",
            MuscleColorSource::Force,
        },
        MuscleColorSourceMetadata{
            "fiber_length",
            "Fiber Length",
            MuscleColorSource::FiberLength,
        },
    });
    static_assert(c_Metadata.size() == osc::num_options<MuscleColorSource>());
}

std::span<const MuscleColorSourceMetadata> opyn::get_all_possible_muscle_coloring_sources_metadata()
{
    return c_Metadata;
}

const MuscleColorSourceMetadata& opyn::get_muscle_coloring_style_metadata(MuscleColorSource s)
{
    return get_all_possible_muscle_coloring_sources_metadata()[get_index_of(s)];
}

ptrdiff_t opyn::get_index_of(MuscleColorSource s)
{
    return static_cast<ptrdiff_t>(s);
}
