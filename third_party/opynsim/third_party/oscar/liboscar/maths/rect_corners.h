#pragma once

#include <liboscar/maths/vector.h>

namespace osc
{
    struct RectCorners final {
        Vector2 min{};
        Vector2 max{};
    };
}
