#pragma once

#include <nonstd/span.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>

namespace osc
{
    enum class MuscleSizingStyle {
        OpenSim = 0,
        PcsaDerived,
        NUM_OPTIONS,

        Default = OpenSim,
    };

    struct MuscleSizingStyleMetadata final {
        CStringView id;
        CStringView label;
        MuscleSizingStyle value;
    };
    nonstd::span<MuscleSizingStyleMetadata const> GetAllMuscleSizingStyleMetadata();
    MuscleSizingStyleMetadata const& GetMuscleSizingStyleMetadata(MuscleSizingStyle);
    ptrdiff_t GetIndexOf(MuscleSizingStyle);
}
