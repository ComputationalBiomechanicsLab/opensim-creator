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
    std::span<const MuscleColorSourceMetadata> GetAllPossibleMuscleColoringSourcesMetadata();
    const MuscleColorSourceMetadata& GetMuscleColoringStyleMetadata(MuscleColorSource);
    ptrdiff_t GetIndexOf(MuscleColorSource);
}
