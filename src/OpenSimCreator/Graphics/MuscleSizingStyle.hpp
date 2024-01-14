#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <span>

namespace osc
{
    enum class MuscleSizingStyle {
        OpenSim,
        PcsaDerived,
        NUM_OPTIONS,

        Default = OpenSim,
    };

    struct MuscleSizingStyleMetadata final {
        CStringView id;
        CStringView label;
        MuscleSizingStyle value;
    };
    std::span<MuscleSizingStyleMetadata const> GetAllMuscleSizingStyleMetadata();
    MuscleSizingStyleMetadata const& GetMuscleSizingStyleMetadata(MuscleSizingStyle);
    ptrdiff_t GetIndexOf(MuscleSizingStyle);
}
