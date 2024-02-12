#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>

namespace osc
{
    enum class MuscleColoringStyle {
        OpenSimAppearanceProperty,
        OpenSim,
        Activation,
        Excitation,
        Force,
        FiberLength,
        NUM_OPTIONS,

        Default = OpenSim,
    };

    struct MuscleColoringStyleMetadata final {
        CStringView id;
        CStringView label;
        MuscleColoringStyle value;
    };
    std::span<MuscleColoringStyleMetadata const> GetAllMuscleColoringStyleMetadata();
    MuscleColoringStyleMetadata const& GetMuscleColoringStyleMetadata(MuscleColoringStyle);
    ptrdiff_t GetIndexOf(MuscleColoringStyle);
}
