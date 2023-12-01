#pragma once

#include <oscar/Maths/Vec2.hpp>

namespace osc
{
    struct Circle final {
        Vec2 origin{};
        float radius = 1.0f;
    };
}
