#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <optional>
#include <string>

namespace osc::lm
{
    struct Landmark final {
        std::optional<std::string> maybeName;
        Vec3 position;
    };
}
