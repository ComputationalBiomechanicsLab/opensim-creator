#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>

namespace osc
{
    enum class MuscleSizingStyle {
        Fixed,
        PcsaDerived,
        NUM_OPTIONS,

        Default = Fixed,
    };

    struct MuscleSizingStyleMetadata final {
        CStringView id;
        CStringView label;
        MuscleSizingStyle value;
    };
    std::span<const MuscleSizingStyleMetadata> GetAllMuscleSizingStyleMetadata();
    const MuscleSizingStyleMetadata& GetMuscleSizingStyleMetadata(MuscleSizingStyle);
    ptrdiff_t GetIndexOf(MuscleSizingStyle);
}
