#pragma once

#include <nonstd/span.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>

namespace osc
{
    enum class MuscleColoringStyle {
        OpenSimAppearanceProperty = 0,
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
    nonstd::span<MuscleColoringStyleMetadata const> GetAllMuscleColoringStyleMetadata();
    MuscleColoringStyleMetadata const& GetMuscleColoringStyleMetadata(MuscleColoringStyle);
    ptrdiff_t GetIndexOf(MuscleColoringStyle);
}
