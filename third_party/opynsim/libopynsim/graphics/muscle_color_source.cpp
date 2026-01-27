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

std::span<const MuscleColorSourceMetadata> opyn::GetAllPossibleMuscleColoringSourcesMetadata()
{
    return c_Metadata;
}

const MuscleColorSourceMetadata& opyn::GetMuscleColoringStyleMetadata(MuscleColorSource s)
{
    return GetAllPossibleMuscleColoringSourcesMetadata()[GetIndexOf(s)];
}

ptrdiff_t opyn::GetIndexOf(MuscleColorSource s)
{
    return static_cast<ptrdiff_t>(s);
}
