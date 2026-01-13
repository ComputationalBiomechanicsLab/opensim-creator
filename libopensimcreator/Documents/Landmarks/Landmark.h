#pragma once

#include <liboscar/maths/Vector3.h>

#include <optional>
#include <string>

namespace osc::lm
{
    struct Landmark final {
        // If available, the name of this landmark.
        std::optional<std::string> maybeName;

        // The position of the landmark in its caller-specified coordinate system.
        Vector3 position;

        friend bool operator==(const Landmark&, const Landmark&) = default;
    };
}
