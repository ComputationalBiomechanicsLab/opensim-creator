#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>

namespace osc
{
    enum class MuscleColorSource {
        OpenSimAppearanceProperty,
        OpenSim,
        Activation,
        Excitation,
        Force,
        FiberLength,
        NUM_OPTIONS,

        Default = OpenSim,
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
