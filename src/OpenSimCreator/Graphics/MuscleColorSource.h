#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>

namespace osc
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
        CStringView id;
        CStringView label;
        MuscleColorSource value;
    };
    std::span<const MuscleColorSourceMetadata> GetAllPossibleMuscleColoringSourcesMetadata();
    const MuscleColorSourceMetadata& GetMuscleColoringStyleMetadata(MuscleColorSource);
    ptrdiff_t GetIndexOf(MuscleColorSource);
}
