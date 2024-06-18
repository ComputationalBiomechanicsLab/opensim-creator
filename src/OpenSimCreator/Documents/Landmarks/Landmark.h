#pragma once

#include <oscar/Maths/Vec3.h>

#include <optional>
#include <string>

namespace osc::lm
{
    struct Landmark final {
        std::optional<std::string> maybeName;
        Vec3 position;

        friend bool operator==(const Landmark&, const Landmark&) = default;
    };
}
