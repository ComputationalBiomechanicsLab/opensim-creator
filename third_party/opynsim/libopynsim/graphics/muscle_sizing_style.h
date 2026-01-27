#pragma once

#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <span>

namespace opyn
{
    enum class MuscleSizingStyle {
        Fixed,
        PcsaDerived,
        NUM_OPTIONS,

        Default = Fixed,
    };

    struct MuscleSizingStyleMetadata final {
        osc::CStringView id;
        osc::CStringView label;
        MuscleSizingStyle value;
    };
    std::span<const MuscleSizingStyleMetadata> GetAllMuscleSizingStyleMetadata();
    const MuscleSizingStyleMetadata& GetMuscleSizingStyleMetadata(MuscleSizingStyle);
    ptrdiff_t GetIndexOf(MuscleSizingStyle);
}
