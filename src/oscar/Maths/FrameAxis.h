#pragma once

#include <iosfwd>
#include <optional>
#include <string_view>

namespace osc
{
    enum class FrameAxis {
        PlusX,
        PlusY,
        PlusZ,
        MinusX,
        MinusY,
        MinusZ,
        NUM_OPTIONS,
    };

    std::optional<FrameAxis> TryParseAsFrameAxis(std::string_view);
    bool AreOrthogonal(FrameAxis, FrameAxis);
    std::ostream& operator<<(std::ostream&, FrameAxis);
}
